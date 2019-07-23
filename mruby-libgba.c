#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/proc.h>
#include <mruby/string.h>

#include <gba.h>

/* ======================================================================= */
/* Resources */
/* ======================================================================= */
typedef struct tagGBAData {
	unsigned int size;
	unsigned char* data;
} GBAData;

/* ======================================================================= */
/* Input */
/* ======================================================================= */
static mrb_value input_update(mrb_state *mrb, mrb_value self) {
	scanKeys();
	return mrb_nil_value();
}

static mrb_value input_keys_down(mrb_state *mrb, mrb_value self) {
	return mrb_fixnum_value(keysDown());
}

static mrb_value input_keys_down_repeat(mrb_state *mrb, mrb_value self) {
	return mrb_fixnum_value(keysDownRepeat());
}

static mrb_value input_keys_up(mrb_state *mrb, mrb_value self) {
	return mrb_fixnum_value(keysUp());
}

static mrb_value input_keys_held(mrb_state *mrb, mrb_value self) {
	return mrb_fixnum_value(keysHeld());
}

/* ======================================================================= */
/* Video */
/* ======================================================================= */
static mrb_value video_rgb5(mrb_state *mrb, mrb_value self) {
	mrb_int r = 0, g = 0, b = 0;
	mrb_get_args(mrb, "iii", &r, &g, &b);
	return mrb_fixnum_value(RGB5(r, g, b));
}

static mrb_value video_rgb8(mrb_state *mrb, mrb_value self) {
	mrb_int r = 0, g = 0, b = 0;
	mrb_get_args(mrb, "iii", &r, &g, &b);
	return mrb_fixnum_value(RGB8(r, g, b));
}

static mrb_value video_write_reg(mrb_state *mrb, mrb_value self) {
	mrb_int address = 0, val = 0;
	if (mrb_get_args(mrb, "ii", &address, &val)) {
		*(char*)address = val;
	}
	return mrb_nil_value();
}

static mrb_value video_set_mode(mrb_state* mrb, mrb_value self) {
	mrb_int mode = 0;
	if (mrb_get_args(mrb, "i", &mode)) {
		SetMode(mode);
	}
	return mrb_nil_value();
}

// Palette
static mrb_value video_palette_bg_set(mrb_state* mrb, mrb_value self) {
	mrb_int index = 0, color = 0;
	if (mrb_get_args(mrb, "ii", &index, &color) == 2) {
		BG_PALETTE[index] = color;
	}
	return mrb_nil_value();
}

static mrb_value video_palette_obj_set(mrb_state* mrb, mrb_value self) {
	mrb_int index = 0, color = 0;
	if (mrb_get_args(mrb, "ii", &index, &color) == 2) {
		SPRITE_PALETTE[index] = color;
	}
	return mrb_nil_value();
}

// Gfx
static mrb_value video_gfx_bg_write(mrb_state* mrb, mrb_value self) {
	mrb_int block = 0, offset = 0;
	mrb_value source;
	if (mrb_get_args(mrb, "iiS", &block, &offset, &source) == 3) {
		dmaCopy(RSTRING_PTR(source), ((u16*)VRAM) + block * 32 + offset, RSTRING_LEN(source));
	}
	return mrb_nil_value();
}

static mrb_value video_gfx_obj_write(mrb_state* mrb, mrb_value self) {
	mrb_int block = 0, offset = 0;
	mrb_value source;
	if (mrb_get_args(mrb, "iiS", &block, &offset, &source) == 3) {
		dmaCopy(RSTRING_PTR(source), SPRITE_GFX + block * 32 + offset, RSTRING_LEN(source));
	}
	return mrb_nil_value();
}

// Mode 3
static mrb_value video_mode3_set_pixel(mrb_state* mrb, mrb_value self) {
	mrb_int x, y, color;
	if (mrb_get_args(mrb, "iii", &x, &y, &color) == 3) {
		((unsigned short int*)VRAM)[y * SCREEN_WIDTH + x] = color;
	}
	return mrb_nil_value();
}

// Mode 4
unsigned short int* mode4_backbuffer = (unsigned short int*)0x06000000;

static mrb_value video_mode4_flip(mrb_state* mrb, mrb_value self) {
	REG_DISPCNT = REG_DISPCNT ^ (1 << 4);
	if (REG_DISPCNT & (1 << 4)) {
		mode4_backbuffer = (unsigned short int*)0x06000000;
	} else {
		mode4_backbuffer = (unsigned short int*)0x0600A000;
	}
	return mrb_nil_value();
}

static mrb_value video_mode4_set_pixel(mrb_state* mrb, mrb_value self) {
	mrb_int x, y, color;
	if (mrb_get_args(mrb, "iii", &x, &y, &color) == 3) {
		register unsigned short int* tc;
		tc = mode4_backbuffer + y * 120 + x / 2;
		if (x & 1) *tc = ((*tc & 0x00FF) + (color << 8));
		else *tc = (*tc & 0xFF00) + color;
	}
	return mrb_nil_value();
}

// OBJ
SpriteEntry obj_buffer[128];

static mrb_value video_obj_update_oam(mrb_state* mrb, mrb_value self) {
	dmaCopy(obj_buffer, OAM, sizeof(OBJATTR) * 128);
	return mrb_nil_value();
}

static mrb_value video_obj_attr0_objy(mrb_state* mrb, mrb_value self) {
	mrb_int n = 0;
	if (mrb_get_args(mrb, "i", &n)) return mrb_fixnum_value(OBJ_Y(n));
	return mrb_nil_value();
}

static mrb_value video_obj_attr1_objx(mrb_state* mrb, mrb_value self) {
	mrb_int n = 0;
	if (mrb_get_args(mrb, "i", &n)) return mrb_fixnum_value(OBJ_X(n));
	return mrb_nil_value();
}

static mrb_value video_obj_attr1_rot(mrb_state* mrb, mrb_value self) {
	mrb_int n = 0;
	if (mrb_get_args(mrb, "i", &n)) return mrb_fixnum_value(ATTR1_ROTDATA(n));
	return mrb_nil_value();
}

static mrb_value video_obj_attr2_priority(mrb_state* mrb, mrb_value self) {
	mrb_int n = 0;
	if (mrb_get_args(mrb, "i", &n)) return mrb_fixnum_value(ATTR2_PRIORITY(n));
	return mrb_nil_value();
}

static mrb_value video_obj_attr2_palette(mrb_state* mrb, mrb_value self) {
	mrb_int n = 0;
	if (mrb_get_args(mrb, "i", &n)) return mrb_fixnum_value(ATTR2_PALETTE(n));
	return mrb_nil_value();
}

static mrb_value video_obj_set_attr(mrb_state* mrb, mrb_value self) {
	mrb_int index, attr, value;
	if (mrb_get_args(mrb, "iii", &index, &attr, &value) == 3) {
		obj_buffer[index].attribute[attr] = value;
	}
	return mrb_nil_value();
}

/* ======================================================================= */
/* Interrupt */
/* ======================================================================= */
static mrb_value interrupt_irq_init(mrb_state *mrb, mrb_value self) {
	irqInit();
	return mrb_nil_value();
}

static mrb_value interrupt_irq_enable(mrb_state *mrb, mrb_value self) {
	mrb_int mask;
	if (mrb_get_args(mrb, "i", &mask)) {
		irqEnable(mask);
	}
	return mrb_nil_value();
}

static mrb_value interrupt_irq_disable(mrb_state *mrb, mrb_value self) {
	mrb_int mask;
	if (mrb_get_args(mrb, "i", &mask)) {
		irqDisable(mask);
	}
	return mrb_nil_value();
}

static mrb_value interrupt_intr_main(mrb_state *mrb, mrb_value self) {
	IntrMain();
	return mrb_nil_value();
}

/* ======================================================================= */
/* SystemCall */
/* ======================================================================= */
static mrb_value systemcalls_soft_reset(mrb_state *mrb, mrb_value self) {
	mrb_int restart_flag;
	if (mrb_get_args(mrb, "i", &restart_flag)) {
		SoftReset(restart_flag);
	}
	return mrb_nil_value();
}

static mrb_value systemcalls_register_ram_reset(mrb_state *mrb, mrb_value self) {
	mrb_int reset_flags;
	if (mrb_get_args(mrb, "i", &reset_flags)) {
		RegisterRamReset(reset_flags);
	}
	return mrb_nil_value();
}

static mrb_value systemcalls_halt(mrb_state *mrb, mrb_value self) {
	Halt();
	return mrb_nil_value();
}

static mrb_value systemcalls_stop(mrb_state *mrb, mrb_value self) {
	Stop();
	return mrb_nil_value();
}

static mrb_value systemcalls_bios_checksum(mrb_state *mrb, mrb_value self) {
	return mrb_fixnum_value(BiosCheckSum());
}

static mrb_value systemcalls_vblank_intr_wait(mrb_state *mrb, mrb_value self) {
	VBlankIntrWait();
	return mrb_nil_value();
}

/* ======================================================================= */
/* SRAM */
/* ======================================================================= */
static mrb_value sram_writebyte(mrb_state* mrb, mrb_value self) {
	mrb_int offset, value;
	if (mrb_get_args(mrb, "ii", &offset, &value) == 2) {
		((char*)SRAM)[offset] = value;
	}
	return mrb_nil_value();
}

static mrb_value sram_readbyte(mrb_state* mrb, mrb_value self) {
	mrb_int offset;
	if (mrb_get_args(mrb, "i", &offset)) {
		return mrb_fixnum_value(((char*)SRAM)[offset]);
	}
	return mrb_nil_value();
}

static mrb_value sram_writestring(mrb_state* mrb, mrb_value self) {
	mrb_int offset;
	mrb_value str;
	if (mrb_get_args(mrb, "iS", &offset, &str) == 2) {
		int i = 0;
		for (;i < RSTRING_LEN(str);i++) {
			((char*)SRAM)[offset + i] = RSTRING_PTR(str)[i];
		}
	}
	return mrb_nil_value();
}

/* ======================================================================= */

#define START arena_size = mrb_gc_arena_save(mrb)
#define DONE mrb_gc_arena_restore(mrb, arena_size)

struct RClass* input = NULL;
struct RClass* video = NULL;

static mrb_value mrb_mruby_libgba_input_init(mrb_state* mrb, mrb_value self) {
	int arena_size;
	// Input
	START;
	#define INPUT_CONST(n) mrb_define_const(mrb, input, #n, mrb_fixnum_value(n))
	INPUT_CONST(KEY_A);
	INPUT_CONST(KEY_B);
	INPUT_CONST(KEY_SELECT);
	INPUT_CONST(KEY_START);
	INPUT_CONST(KEY_RIGHT);
	INPUT_CONST(KEY_LEFT);
	INPUT_CONST(KEY_UP);
	INPUT_CONST(KEY_DOWN);
	INPUT_CONST(KEY_R);
	INPUT_CONST(KEY_L);
	INPUT_CONST(DPAD);
	#undef INPUT_CONST
	DONE;
	mrb_define_class_method(mrb, input, "update", input_update, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, input, "keys_down", input_keys_down, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, input, "keys_down_repeat", input_keys_down_repeat, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, input, "keys_held", input_keys_held, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, input, "keys_up", input_keys_up, MRB_ARGS_NONE());
	return mrb_nil_value();
}

static mrb_value mrb_mruby_libgba_video_init(mrb_state* mrb, mrb_value self) {
	int arena_size;
	struct RClass* video_palette = mrb_define_module_under(mrb, video, "Palette");
	struct RClass* video_gfx = mrb_define_module_under(mrb, video, "Gfx");
	struct RClass* video_obj = mrb_define_module_under(mrb, video, "OBJ");
	struct RClass* video_mode3 = mrb_define_module_under(mrb, video, "Mode3");
	struct RClass* video_mode4 = mrb_define_module_under(mrb, video, "Mode4");
	// Video
	mrb_define_class_method(mrb, video, "rgb5", video_rgb5, MRB_ARGS_REQ(3));
	mrb_define_class_method(mrb, video, "rgb8", video_rgb8, MRB_ARGS_REQ(3));
	mrb_define_class_method(mrb, video, "write_reg", video_write_reg, MRB_ARGS_REQ(2));
	
	START;
	
	mrb_define_const(mrb, video, "SCREEN_WIDTH", mrb_fixnum_value(SCREEN_WIDTH));
	mrb_define_const(mrb, video, "SCREEN_HEIGHT", mrb_fixnum_value(SCREEN_HEIGHT));
	
	mrb_define_const(mrb, video, "REG_MOSAIC", mrb_fixnum_value(REG_MOSAIC));
	
	mrb_define_const(mrb, video, "MODE_0", mrb_fixnum_value(MODE_0));
	mrb_define_const(mrb, video, "MODE_1", mrb_fixnum_value(MODE_1));
	mrb_define_const(mrb, video, "MODE_2", mrb_fixnum_value(MODE_2));
	mrb_define_const(mrb, video, "MODE_3", mrb_fixnum_value(MODE_3));
	mrb_define_const(mrb, video, "MODE_4", mrb_fixnum_value(MODE_4));
	mrb_define_const(mrb, video, "MODE_5", mrb_fixnum_value(MODE_5));
	mrb_define_const(mrb, video, "OBJ_1D_MAP", mrb_fixnum_value(OBJ_1D_MAP));
	mrb_define_const(mrb, video, "BG0_ENABLE", mrb_fixnum_value(BG0_ENABLE));
	mrb_define_const(mrb, video, "BG1_ENABLE", mrb_fixnum_value(BG1_ENABLE));
	mrb_define_const(mrb, video, "BG2_ENABLE", mrb_fixnum_value(BG2_ENABLE));
	mrb_define_const(mrb, video, "BG3_ENABLE", mrb_fixnum_value(BG3_ENABLE));
	mrb_define_const(mrb, video, "OBJ_ENABLE", mrb_fixnum_value(OBJ_ENABLE));
	mrb_define_const(mrb, video, "WIN0_ENABLE", mrb_fixnum_value(WIN0_ENABLE));
	mrb_define_const(mrb, video, "WIN1_ENABLE", mrb_fixnum_value(WIN1_ENABLE));
	mrb_define_const(mrb, video, "OBJ_WIN_ENABLE", mrb_fixnum_value(OBJ_WIN_ENABLE));
	mrb_define_const(mrb, video, "BG_ALL_ENABLE", mrb_fixnum_value(BG_ALL_ENABLE));
	mrb_define_class_method(mrb, video, "set_mode", video_set_mode, MRB_ARGS_REQ(1));
	
	DONE;
	
	// Video_Palette
	mrb_define_class_method(mrb, video_palette, "bg_set", video_palette_bg_set, MRB_ARGS_REQ(2));
	mrb_define_class_method(mrb, video_palette, "obj_set", video_palette_obj_set, MRB_ARGS_REQ(2));
	
	// Video_Gfx
	mrb_define_class_method(mrb, video_gfx, "bg_write", video_gfx_bg_write, MRB_ARGS_REQ(3));
	mrb_define_class_method(mrb, video_gfx, "obj_write", video_gfx_obj_write, MRB_ARGS_REQ(3));
	
	// Video_Mode3
	mrb_define_class_method(mrb, video_mode3, "set_pixel", video_mode3_set_pixel, MRB_ARGS_REQ(3));
	
	// Video_Mode4
	mrb_define_class_method(mrb, video_mode4, "flip", video_mode4_flip, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, video_mode4, "set_pixel", video_mode4_set_pixel, MRB_ARGS_REQ(3));

	// Video_OBJ
	{
		int i = 0;
		for (; i < 128; i++) obj_buffer[i].attribute[0] = ATTR0_DISABLED;
	}
	mrb_define_class_method(mrb, video_obj, "update_oam", video_obj_update_oam, MRB_ARGS_NONE());
	
	START;
	#define VIDEO_OBJ_CONST(n) mrb_define_const(mrb, video_obj, #n, mrb_fixnum_value(n))
	VIDEO_OBJ_CONST(ATTR0_MOSAIC);
	VIDEO_OBJ_CONST(ATTR0_COLOR_256);
	VIDEO_OBJ_CONST(ATTR0_COLOR_16);
	VIDEO_OBJ_CONST(ATTR0_TYPE_NORMAL);
	VIDEO_OBJ_CONST(ATTR0_TYPE_BLENDED);
	VIDEO_OBJ_CONST(ATTR0_TYPE_WINDOWED);
	VIDEO_OBJ_CONST(ATTR0_NORMAL);
	VIDEO_OBJ_CONST(ATTR0_ROTSCALE);
	VIDEO_OBJ_CONST(ATTR0_DISABLED);
	VIDEO_OBJ_CONST(ATTR0_ROTSCALE_DOUBLE);
	VIDEO_OBJ_CONST(ATTR0_SQUARE);
	VIDEO_OBJ_CONST(ATTR0_WIDE);
	VIDEO_OBJ_CONST(ATTR0_TALL);
	VIDEO_OBJ_CONST(ATTR1_FLIP_X);
	VIDEO_OBJ_CONST(ATTR1_FLIP_Y);
	VIDEO_OBJ_CONST(ATTR1_SIZE_8);
	VIDEO_OBJ_CONST(ATTR1_SIZE_16);
	VIDEO_OBJ_CONST(ATTR1_SIZE_32);
	VIDEO_OBJ_CONST(ATTR1_SIZE_64);
	#undef VIDEO_OBJ_CONST
	DONE;
	
	mrb_define_class_method(mrb, video_obj, "attr0_objy", video_obj_attr0_objy, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, video_obj, "attr1_objx", video_obj_attr1_objx, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, video_obj, "attr1_rot", video_obj_attr1_rot, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, video_obj, "attr2_priority", video_obj_attr2_priority, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, video_obj, "attr2_palette", video_obj_attr2_palette, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, video_obj, "set_attr", video_obj_set_attr, MRB_ARGS_REQ(3));
	
	return mrb_nil_value();
}

void mrb_mruby_libgba_gem_init(mrb_state* mrb) {
	int arena_size;
	
	struct RClass* interrupt = mrb_define_module(mrb, "Interrupt");
	struct RClass* systemcalls = mrb_define_module(mrb, "Systemcalls");
	struct RClass* sram = mrb_define_module(mrb, "SRAM");
	
	input = mrb_define_module(mrb, "Input");
	video = mrb_define_module(mrb, "Video");
	
	// Video & Input
	mrb_define_class_method(mrb, video, "install", mrb_mruby_libgba_video_init, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, input, "install", mrb_mruby_libgba_input_init, MRB_ARGS_NONE());
	
	// Interrupt
	START;
	mrb_define_const(mrb, interrupt, "IRQ_VBLANK", mrb_fixnum_value(IRQ_VBLANK));
	mrb_define_const(mrb, interrupt, "IRQ_HBLANK", mrb_fixnum_value(IRQ_HBLANK));
	mrb_define_const(mrb, interrupt, "IRQ_VCOUNT", mrb_fixnum_value(IRQ_VCOUNT));
	mrb_define_const(mrb, interrupt, "IRQ_TIMER0", mrb_fixnum_value(IRQ_TIMER0));
	mrb_define_const(mrb, interrupt, "IRQ_TIMER1", mrb_fixnum_value(IRQ_TIMER1));
	mrb_define_const(mrb, interrupt, "IRQ_TIMER2", mrb_fixnum_value(IRQ_TIMER2));
	mrb_define_const(mrb, interrupt, "IRQ_TIMER3", mrb_fixnum_value(IRQ_TIMER3));
	mrb_define_const(mrb, interrupt, "IRQ_SERIAL", mrb_fixnum_value(IRQ_SERIAL));
	mrb_define_const(mrb, interrupt, "IRQ_DMA0", mrb_fixnum_value(IRQ_DMA0));
	mrb_define_const(mrb, interrupt, "IRQ_DMA1", mrb_fixnum_value(IRQ_DMA1));
	mrb_define_const(mrb, interrupt, "IRQ_DMA2", mrb_fixnum_value(IRQ_DMA2));
	mrb_define_const(mrb, interrupt, "IRQ_DMA3", mrb_fixnum_value(IRQ_DMA3));
	mrb_define_const(mrb, interrupt, "IRQ_KEYPAD", mrb_fixnum_value(IRQ_KEYPAD));
	mrb_define_const(mrb, interrupt, "IRQ_GAMEPAK", mrb_fixnum_value(IRQ_GAMEPAK));
	DONE;
	
	mrb_define_class_method(mrb, interrupt, "irq_init", interrupt_irq_init, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, interrupt, "irq_enable", interrupt_irq_enable, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, interrupt, "irq_disable", interrupt_irq_disable, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, interrupt, "intr_main", interrupt_intr_main, MRB_ARGS_NONE());
	
	
	// SystemCall
	START;
	
	mrb_define_const(mrb, systemcalls, "ROM_RESTART", mrb_fixnum_value(ROM_RESTART));
	mrb_define_const(mrb, systemcalls, "RAM_RESTART", mrb_fixnum_value(RAM_RESTART));
	
	mrb_define_const(mrb, systemcalls, "RESET_EWRAM", mrb_fixnum_value(RESET_EWRAM));
	mrb_define_const(mrb, systemcalls, "RESET_IWRAM", mrb_fixnum_value(RESET_IWRAM));
	mrb_define_const(mrb, systemcalls, "RESET_PALETTE", mrb_fixnum_value(RESET_PALETTE));
	mrb_define_const(mrb, systemcalls, "RESET_VRAM", mrb_fixnum_value(RESET_VRAM));
	mrb_define_const(mrb, systemcalls, "RESET_OAM", mrb_fixnum_value(RESET_OAM));
	mrb_define_const(mrb, systemcalls, "RESET_SIO", mrb_fixnum_value(RESET_SIO));
	mrb_define_const(mrb, systemcalls, "RESET_SOUND", mrb_fixnum_value(RESET_SOUND));
	mrb_define_const(mrb, systemcalls, "RESET_OTHER", mrb_fixnum_value(RESET_OTHER));
	
	DONE;
	
	mrb_define_class_method(mrb, systemcalls, "soft_reset", systemcalls_soft_reset, MRB_ARGS_REQ(1));
	
	mrb_define_class_method(mrb, systemcalls, "register_ram_reset", systemcalls_register_ram_reset, MRB_ARGS_REQ(1));
	
	mrb_define_class_method(mrb, systemcalls, "halt", systemcalls_halt, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, systemcalls, "stop", systemcalls_stop, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, systemcalls, "bios_checksum", systemcalls_bios_checksum, MRB_ARGS_NONE());
	mrb_define_class_method(mrb, systemcalls, "vblank_intr_wait", systemcalls_vblank_intr_wait, MRB_ARGS_NONE());
	
	// Sram
	mrb_define_class_method(mrb, sram, "write_byte", sram_writebyte, MRB_ARGS_REQ(2));
	mrb_define_class_method(mrb, sram, "read_byte", sram_readbyte, MRB_ARGS_REQ(1));
	mrb_define_class_method(mrb, sram, "write_string", sram_writestring, MRB_ARGS_REQ(2));
	
	// struct RClass *c;

	// c = mrb_define_class(mrb, "Fiber", mrb->object_class);
	// MRB_SET_INSTANCE_TT(c, MRB_TT_FIBER);

	// mrb_define_method(mrb, c, "initialize", fiber_init,    MRB_ARGS_NONE());
	// mrb_define_method(mrb, c, "resume",     fiber_resume,  MRB_ARGS_ANY());
	// mrb_define_method(mrb, c, "transfer",   fiber_transfer, MRB_ARGS_ANY());
	// mrb_define_method(mrb, c, "alive?",     fiber_alive_p, MRB_ARGS_NONE());
	// mrb_define_method(mrb, c, "==",         fiber_eq,      MRB_ARGS_REQ(1));

	// mrb_define_class_method(mrb, c, "yield", fiber_yield, MRB_ARGS_ANY());
	// mrb_define_class_method(mrb, c, "current", fiber_current, MRB_ARGS_NONE());

	// mrb_define_class(mrb, "FiberError", mrb->eStandardError_class);
}
#undef START
#undef DONE

void mrb_mruby_libgba_gem_final(mrb_state* mrb) {
	
}
