//
//
//
#include "../src/stuff.hpp"
#include <chrono>
#include <libgbc/machine.hpp>
using buffer_t = std::vector<uint8_t>;

static const int SNAPSHOT_INTERVAL = 512;

struct snapshot_t
{
    uint32_t progress = 0;
    uint32_t frame = 0;
    buffer_t state;
    buffer_t inputs;
    size_t improve_size = 0;

    bool validate(const uint32_t death_progress) const noexcept
    {
        return death_progress - progress > 100;
    }
    bool improvement(const snapshot_t& other) const noexcept
    {
        if (this->progress == other.progress) return this->frame < other.frame;
        return false;
    }
    bool better(const snapshot_t& other) const noexcept { return this->progress > other.progress; }
    void append(const snapshot_t& other, bool improvement)
    {
        // set new progress
        this->progress = other.progress;
        this->frame = other.frame;
        // overwrite machine state
        this->state = other.state;
        // for improvements go back to old size
        if (improvement) { inputs.resize(this->improve_size); }
        // to be able to improve this snapshot we must remember the old size
        this->improve_size = other.inputs.size();
        // append new snapshot inputs to current
        inputs.insert(inputs.end(), other.inputs.begin(), other.inputs.end());
    }
    void append_inputs(const buffer_t& more_inputs)
    {
        inputs.insert(inputs.end(), more_inputs.begin(), more_inputs.end());
    }
};

struct training_results_t
{
    enum verdict_t
    {
        DEATH,
        STUCK,
        TIMEOUT,
        FINISH
    };

    buffer_t inputs;
    uint64_t frame = 0;
    uint32_t progress = 0;
    verdict_t verdict = DEATH;

    snapshot_t snapshot;

    bool operator<(const training_results_t& other) { return this->frame < other.frame; }
};

struct Worker
{
    void setup_callbacks(gbc::Machine& machine);
    void simulate_running(gbc::Machine& machine);

    const int tidx;
    bool started = false;
    training_results_t result;
};

void Worker::setup_callbacks(gbc::Machine& machine)
{
    machine.io.on_joypad_read([this](gbc::Machine& machine, int mode) {
        if (mode == 0)
        {
            // printf("%zu: Machine is about to read buttons\n", frame);
        }
        else
        {
            // printf("%zu: Machine is about to read dpad\n", frame);
            this->simulate_running(machine);
        }
    });
    // check progress on each V-blank
    machine.set_handler(gbc::Machine::VBLANK, [this](gbc::Machine& machine, gbc::interrupt_t&) {
        if (UNLIKELY(started == false))
        {
            if (machine.memory.read8(0xA22C) == 5)
            {
                // printf("A22C is 5 at frame %zu\n", frame);
                this->started = true;
            }
        }
        const uint16_t progress = machine.memory.read16(0xFFC2);
        // record a snapshot each progress interval
        if (progress % SNAPSHOT_INTERVAL == 0)
        {
            auto& snapshot = this->result.snapshot;
            snapshot.progress = progress;
            snapshot.frame = machine.gpu.frame_count();
            snapshot.state.clear();
            machine.serialize_state(snapshot.state);
            snapshot.inputs = result.inputs;
        }
    });
}

// platformer running simulation
void Worker::simulate_running(gbc::Machine& machine)
{
    // memory locations:
    // LIFE (A043) A22C == 5
    const uint64_t frame = machine.gpu.frame_count();
    const double t = frame * 0.0167;
    // printf("%zu: Machine is about to read dpad\n", frame);
    // NOTE: to save bytes lets only record for dpad
    const uint16_t SCROLL_X = machine.memory.read16(0xFFC2);
    if (this->started)
    {
        const uint8_t SCX = machine.memory.read8(gbc::IO::REG_SCX);
        // stuck detection using SCX register
        thread_local uint16_t last_scroll = 0;
        thread_local size_t stuck_detect = 0;
        if (last_scroll == SCX)
        {
            if (stuck_detect++ >= 500)
            {
                printf("T=%d *STUCK* for %zu frames at frame %zu\n", tidx, stuck_detect, frame);
                result.verdict = training_results_t::STUCK;
                machine.stop();
            }
        }
        else
        {
            last_scroll = SCX;
            stuck_detect = 0;
        }
        // Timeout detection
        if (t > 120.0)
        {
            printf("T=%d *TIMEOUT* detected at frame %zu\n", tidx, frame);
            result.verdict = training_results_t::TIMEOUT;
            machine.stop();
            return;
        }
        // Mario sprite change detection
        if (t > 6.0)
        {
            bool death_detected = false;
            for (const auto* spr = machine.gpu.sprites_begin(); spr < machine.gpu.sprites_end();
                 spr++)
            {
                if (!spr->hidden() && spr->pattern_idx() == 0x4E)
                {
                    death_detected = true;
                    break;
                }
            }
            if (death_detected)
            {
                printf("T=%d *DEATH* *SPRITE* detected at frame %zu\n", tidx, frame);
                result.verdict = training_results_t::DEATH;
                machine.stop();
                return;
            }
        }

        if (SCROLL_X >= 4000)
        {
            printf("T=%d Finish registered at frame %zu SCROLL_X %u\n", tidx, frame, SCROLL_X);
            result.verdict = training_results_t::FINISH;
            machine.stop();
        }
    }
    uint8_t jpad = 0;
    // use START to get into the level
    if (t < 4.0) { jpad |= (frame % 2) ? gbc::BUTTON_START : 0; }
    else
    {
        jpad = gbc::BUTTON_B;
        if (rand() % 10) jpad |= gbc::BUTTON_A;
        jpad |= gbc::DPAD_RIGHT;
    }
    // record
    machine.set_inputs(jpad);
    result.inputs.push_back(jpad);
    result.frame = frame;
    result.progress = SCROLL_X;
}

// record gameboy input state
static void write_recorded_state(const buffer_t& inputs)
{
    FILE* outf = fopen("output.gis", "wb");
    assert(outf != nullptr);
    fwrite(inputs.data(), inputs.size(), 1, outf);
    fclose(outf);
    printf("\n* Recorded %zu bytes of inputs\n", inputs.size());
}

#include <future>
#include <thread>
static training_results_t training_session(const int tidx, const buffer_t& romdata,
                                           const buffer_t machine_state)
{
    gbc::Machine machine{romdata};
    machine.gpu.scanline_rendering(false);
    if (!machine_state.empty()) { machine.restore_state(machine_state); }

    Worker thread_ctx{.tidx = tidx};
    thread_ctx.setup_callbacks(machine);

    while (machine.is_running()) { machine.simulate(); }

    return std::move(thread_ctx.result);
}

int main(int argc, char** args)
{
    const char* romfile = "../smbland2_dx.gbc";
    if (argc >= 2) romfile = args[1];

    const auto romdata = load_file(romfile);
    printf("Loaded %zu bytes ROM\n", romdata.size());

    srand(time(0));

    static const int NUM_THREADS = 4;
    std::array<std::future<training_results_t>, NUM_THREADS> futures;
    std::array<training_results_t, NUM_THREADS> results;
    snapshot_t best_snapshot;

    while (true)
    {
        for (size_t i = 0; i < NUM_THREADS; i++)
        {
            futures.at(i) = std::async(std::launch::async, training_session, i + 1, romdata,
                                       best_snapshot.state);
        }
        for (size_t i = 0; i < NUM_THREADS; i++)
        {
            results.at(i) = futures.at(i).get();
            const auto& result = results.at(i);
            if (result.verdict == training_results_t::FINISH)
            {
                printf("*** Final result frame %zu\n", result.frame);
                best_snapshot.append_inputs(result.inputs);
                best_snapshot.inputs.push_back(0); // disable inputs
                write_recorded_state(best_snapshot.inputs);
                return 0;
            }
            // check if there is a decent snapshot
            if (result.snapshot.validate(result.progress))
            {
                if (result.snapshot.better(best_snapshot))
                {
                    best_snapshot.append(result.snapshot, false);
                    printf("*** New best snapshot at progress %u\n", best_snapshot.progress);
                }
                else if (result.snapshot.improvement(best_snapshot))
                {
                    best_snapshot.append(result.snapshot, true);
                    printf("*** New improved snapshot at progress %u\n", best_snapshot.progress);
                }
                // write_recorded_state(best_snapshot.inputs);
            }
        }
    }
    // auto best = std::min_element(std::begin(results), std::end(results));
    // printf("Final result frame %zu\n", best->frame);
    return 0;
}
