#include "varnish.h"
#include <cstdio>
#include <libgbc/machine.hpp>
#include <spng.h>

EMBED_BINARY(index_html, "../index.html");
EMBED_BINARY(rom, "../rom.gbc");
static std::vector<uint8_t> romdata { rom, rom + rom_size };

using PaletteArray = struct spng_plte;
struct PixelState {
	PaletteArray palette;
};
using InputState = uint8_t;
static gbc::Machine* machine = nullptr;
static PixelState storage_state;
static std::pair<void*, size_t> png {nullptr, 0};

static std::pair<void*, size_t>
generate_png(const std::vector<uint8_t>& pixels, PaletteArray& palette)
{
    const int size_x = 160;
    const int size_y = 144;

	// Render to PNG
	static spng_ctx* enc = nullptr;
	if (enc) {
		spng_ctx_free(enc);
	}
	enc = spng_ctx_new(SPNG_CTX_ENCODER);
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

	return {png_buf, png_size};
}

struct FrameState {
	size_t   frame_number;
	timespec ts;
	InputState inputs;
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

static void get_state(size_t n, struct virtbuffer vb[n], size_t res)
{
	assert(machine);

	auto& inputs = *(InputState*)vb[0].data;
	current_state.inputs |= inputs;

	auto t1 = time_now();

	if (time_diff(current_state.ts, t1) > 0.016)
	{
		machine->set_inputs(current_state.inputs);

		machine->simulate_one_frame();
		current_state.frame_number = machine->gpu.frame_count();
		current_state.ts = t1;
		current_state.inputs = {};
		png = generate_png(machine->gpu.pixels(), storage_state.palette);
	}

	storage_return(png.first, png.second);
}

static void on_get(const char* c_url, int, int)
{
	std::string url = c_url;

	if (url == "/x") {
		const char* ctype = "text/html";
		backend_response(200, ctype, strlen(ctype),
			index_html, index_html_size);
	}

	InputState inputs {};
	if (url.find('a') != std::string::npos) inputs |= gbc::BUTTON_A;
	if (url.find('b') != std::string::npos) inputs |= gbc::BUTTON_B;
	if (url.find('e') != std::string::npos) inputs |= gbc::BUTTON_START;
	if (url.find('s') != std::string::npos) inputs |= gbc::BUTTON_SELECT;
	if (url.find('u') != std::string::npos) inputs |= gbc::DPAD_UP;
	if (url.find('d') != std::string::npos) inputs |= gbc::DPAD_DOWN;
	if (url.find('r') != std::string::npos) inputs |= gbc::DPAD_RIGHT;
	if (url.find('l') != std::string::npos) inputs |= gbc::DPAD_LEFT;

	// Read the current state from the shared storage VM
	// Input: Input state from this request
	// Output: Encoded indexed PNG
	char output[8192];
	ssize_t output_len =
		storage_call(get_state, &inputs, sizeof(inputs), output, sizeof(output));

	const char* ctype = "image/png";
	backend_response(200, ctype, strlen(ctype), output, output_len);
}

static void do_serialize_state() {
	std::vector<uint8_t> state;
	machine->serialize_state(state);
	state.insert(state.end(), (uint8_t*) &storage_state, (uint8_t*) &storage_state + sizeof(storage_state));
	state.insert(state.end(), (uint8_t*) &current_state, (uint8_t*) &current_state + sizeof(FrameState));
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

int main(int argc, char** argv)
{
	// Storage has a gbc emulator
	if (std::string(argv[2]) == "1")
	{
		machine = new gbc::Machine(romdata);
		machine->gpu.on_palchange([](const uint8_t idx, const uint16_t color) {
			// The compiler will probably optimize this just fine if
			// we do it the proper hard way, anyway.
			auto& entry = storage_state.palette.entries[idx];
	        *(uint32_t *)&entry = gbc::GPU::color15_to_rgba32(color);
	    });

		current_state.frame_number = 0;
		current_state.ts = time_now();

		printf("Done loading\n");
		fflush(stdout);
	}

	set_backend_get(on_get);
	set_on_live_update(do_serialize_state);
	set_on_live_restore(do_restore_state);
	wait_for_requests();
}
