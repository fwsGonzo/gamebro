#include "varnish.h"
#include <cstdio>
#include <libgbc/machine.hpp>
#include <spng.h>
#include "file_helpers.cpp"

EMBED_BINARY(index_html, "../index.html");
EMBED_BINARY(rom, "../rom.gbc");
static std::vector<uint8_t> romdata { rom, rom + rom_size };
static std::string statefile = "";

using PaletteArray = struct spng_plte;
struct PixelState {
	PaletteArray palette;
};
struct InputState {
	void contribute(uint8_t keys) {
		for (size_t i = 0; i < keystate.size(); i++) {
			uint8_t idx = (current + i) % keystate.size();
			keystate.at(idx) |= keys;
		}
		contribs += 1;
	}
	uint8_t get() const {
		uint8_t keys = keystate.at(current);
		// Prevent buggy behavior from impossible dpad states
		if (keys & gbc::DPAD_UP) keys &= ~gbc::DPAD_DOWN;
		if (keys & gbc::DPAD_RIGHT) keys &= ~gbc::DPAD_LEFT;
		return keys;
	}
	auto contributors() const {
		return contribs;
	}
	void next() {
		keystate.at(current) = 0;
		current = (current + 1) % keystate.size();
		contribs = 0;
	}

	std::array<uint8_t, 4> keystate;
	uint8_t current = 0;
	uint16_t contribs;
};
static gbc::Machine* machine = nullptr;
static PixelState storage_state;
static std::pair<void*, size_t> png {nullptr, 0};
static struct {
	gbc::Machine* machine = nullptr;
	PaletteArray palette;
	uint8_t forwarded_keys = 0;
	std::pair<void*, size_t> png {nullptr, 0};
	uint64_t predictions = 0;
	uint64_t predicted = 0;
} predict;

static std::pair<void*, size_t>
generate_png(const std::vector<uint8_t>& pixels, PaletteArray& palette)
{
    const int size_x = 160;
    const int size_y = 144;

	// Render to PNG
	spng_ctx* enc = spng_ctx_new(SPNG_CTX_ENCODER);
	spng_set_option(enc, SPNG_ENCODE_TO_BUFFER, 1);
	spng_set_crc_action(enc, SPNG_CRC_USE, SPNG_CRC_USE);

	spng_ihdr ihdr;
	spng_get_ihdr(enc, &ihdr);
	ihdr.width = size_x;
	ihdr.height = size_y;
	ihdr.color_type = SPNG_COLOR_TYPE_INDEXED;
	ihdr.bit_depth = 8;

	spng_set_ihdr(enc, &ihdr);
	palette.n_entries = 64;
	spng_set_plte(enc, &palette);

	int ret =
		spng_encode_image(enc,
			pixels.data(), pixels.size(),
			SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
	assert(ret == 0);

	size_t png_size = 0;
    void  *png_buf = spng_get_png_buffer(enc, &png_size, &ret);

	spng_ctx_free(enc);

	return {png_buf, png_size};
}

struct FrameState {
	timespec ts;
	InputState inputs;
	uint16_t contribs = 0;
};
static FrameState current_state;

static timespec time_now() {
	timespec t;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
	return t;
}
static double time_diff(timespec start_time, timespec end_time) {
	const double secs = end_time.tv_sec - start_time.tv_sec;
	return secs + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
}

static void predict_next_frame(void*)
{
	if (predict.machine == nullptr) {
		predict.machine = new gbc::Machine(romdata);
	}
	predict.predictions ++;

	// Serialize state from main GBC and restore it to the prediction GBC
	std::vector<uint8_t> v;
	machine->serialize_state(v);
	predict.machine->restore_state(v);

	predict.machine->gpu.on_palchange([](const uint8_t idx, const uint16_t color) {
		auto& entry = predict.palette.entries[idx];
		*(uint32_t *)&entry = gbc::GPU::color15_to_rgba32(color);
	});
	predict.machine->set_inputs(predict.forwarded_keys);
	predict.machine->simulate_one_frame();

	// Encode predicted PNG
	std::free(predict.png.first);
	predict.png = generate_png(predict.machine->gpu.pixels(), predict.palette);
}

static void get_frame(size_t n, struct virtbuffer vb[n], size_t res)
{
	assert(machine);

	auto& inputs = *(uint8_t*)vb[0].data;
	current_state.inputs.contribute(inputs);

	auto t1 = time_now();

	if (time_diff(current_state.ts, t1) > 0.016)
	{
		auto keys = current_state.inputs.get();

		if (predict.forwarded_keys == keys && predict.machine) {
			std::swap(machine, predict.machine);
			storage_state.palette = predict.palette;
			std::swap(png, predict.png);
			predict.predicted ++;
		} else {
			machine->set_inputs(keys);
			machine->gpu.on_palchange([](const uint8_t idx, const uint16_t color) {
				// The compiler will probably optimize this just fine if
				// we do it the proper hard way, anyway.
				auto& entry = storage_state.palette.entries[idx];
		        *(uint32_t *)&entry = gbc::GPU::color15_to_rgba32(color);
		    });
			machine->simulate_one_frame();

			// Encode new PNG
			std::free(png.first);
			png = generate_png(machine->gpu.pixels(), storage_state.palette);
		}

		current_state.ts = t1;
		current_state.contribs = current_state.inputs.contributors();
		current_state.inputs.next();

		// Predict next frame
		predict.forwarded_keys = current_state.inputs.get();
		predict.palette = storage_state.palette;
		async_storage_task(predict_next_frame, &predict);

		// Store state every X frames
		if (machine->gpu.frame_count() % 32 == 0)
		{
			auto state = create_serialized_state();
			file_writer(statefile, state);
		}
	}

	std::string prsstr = "X-Predictions: " + std::to_string(predict.predictions);
	http_append(5, prsstr.data(), prsstr.size());
	std::string prdstr = "X-Predicted: " + std::to_string(predict.predicted);
	http_append(5, prdstr.data(), prdstr.size());

	std::string frcstr = "X-FrameCount: " + std::to_string(machine->gpu.frame_count());
	http_append(5, frcstr.data(), frcstr.size());
	std::string ctrstr = "X-Contributors: " + std::to_string(current_state.contribs);
	http_append(5, ctrstr.data(), ctrstr.size());

	storage_return(png.first, png.second);
}

static void on_get(const char* c_url, int, int resp)
{
	std::string url = c_url;

	if (url == "/x") {
		set_cacheable(true, 3600.0);
		const char* ctype = "text/html";
		backend_response(200, ctype, strlen(ctype),
			index_html, index_html_size);
	}

	uint8_t inputs = 0;
	if (url.find('a') != std::string::npos) inputs |= gbc::BUTTON_A;
	if (url.find('b') != std::string::npos) inputs |= gbc::BUTTON_B;
	if (url.find('e') != std::string::npos) inputs |= gbc::BUTTON_START;
	if (url.find('s') != std::string::npos) inputs |= gbc::BUTTON_SELECT;
	if (url.find('u') != std::string::npos) inputs |= gbc::DPAD_UP;
	if (url.find('d') != std::string::npos) inputs |= gbc::DPAD_DOWN;
	if (url.find('r') != std::string::npos) inputs |= gbc::DPAD_RIGHT;
	if (url.find('l') != std::string::npos) inputs |= gbc::DPAD_LEFT;

	// Disable client-side caching
	const char* nostore = "cache-control: no-store";
	http_append(resp, nostore, strlen(nostore));

	set_cacheable(false, 8.0);

	// Read the current frame from the shared storage VM
	// Input: Input state from this request
	// Output: Encoded indexed 32-bit PNG
	char output[6000];
	ssize_t output_len =
		storage_call(get_frame, &inputs, sizeof(inputs), output, sizeof(output));

	const char* ctype = "image/png";
	backend_response(200, ctype, strlen(ctype), output, output_len);
}

static std::vector<uint8_t> create_serialized_state()
{
	std::vector<uint8_t> state;
	machine->serialize_state(state);
	state.insert(state.end(), (uint8_t*) &storage_state, (uint8_t*) &storage_state + sizeof(storage_state));
	state.insert(state.end(), (uint8_t*) &current_state, (uint8_t*) &current_state + sizeof(FrameState));
	return state;
}
static void restore_state_from(const std::vector<uint8_t> state)
{
	size_t off = machine->restore_state(state);
	if (state.size() >= off + sizeof(PixelState)) {
		storage_state = *(PixelState*) &state.at(off);
		off += sizeof(PixelState);
	}
	if (state.size() >= off + sizeof(FrameState)) {
		current_state = *(FrameState*) &state.at(off);
	}
	printf("State restored!\n");
	fflush(stdout);
}
static void do_serialize_state() {
	auto state = create_serialized_state();
	storage_return(state.data(), state.size());
}
static void do_restore_state(size_t len) {
	printf("State: %zu bytes\n", len);
	fflush(stdout);
	// Restoration happens in two stages.
	// 1st stage: No data, but the length is provided.
	std::vector<uint8_t> state;
	state.resize(len);
	storage_return(state.data(), state.size());
	// 2nd stage: Do the actual restoration:
	restore_state_from(state);
}

int main(int argc, char** argv)
{
	// Storage has a gbc emulator
	if (std::string(argv[2]) == "1")
	{
		machine = new gbc::Machine(romdata);

		current_state.ts = time_now();

		statefile = argv[3];
		try {
			auto state = file_loader(statefile);
			if (!state.empty()) {
				restore_state_from(state);
			}
		} catch (...) {
			fflush(stdout);
		}

		printf("Done loading\n");
		fflush(stdout);
	}

	set_backend_get(on_get);
	set_on_live_update(do_serialize_state);
	set_on_live_restore(do_restore_state);
	wait_for_requests();
}
