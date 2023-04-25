#include "Chip8.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"

#include "files.hpp"

#include <ctime>

#define V0 _v[0x0]
#define V1 _v[0x1]
#define V2 _v[0x2]
#define V3 _v[0x3]
#define V4 _v[0x4]
#define V5 _v[0x5]
#define V6 _v[0x6]
#define V7 _v[0x7]
#define V8 _v[0x8]
#define V9 _v[0x9]
#define VA _v[0xa]
#define VB _v[0xb]
#define VC _v[0xc]
#define VD _v[0xd]
#define VE _v[0xe]
#define VF _v[0xf]

#define FONTSET_SIZE 80
const unsigned char fontset[FONTSET_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,		// 0
	0x20, 0x60, 0x20, 0x20, 0x70,		// 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0,		// 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0,		// 3
	0x90, 0x90, 0xF0, 0x10, 0x10,		// 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0,		// 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0,		// 6
	0xF0, 0x10, 0x20, 0x40, 0x40,		// 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0,		// 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0,		// 9
	0xF0, 0x90, 0xF0, 0x90, 0x90,		// A
	0xE0, 0x90, 0xE0, 0x90, 0xE0,		// B
	0xF0, 0x80, 0x80, 0x80, 0xF0,		// C
	0xE0, 0x90, 0x90, 0x90, 0xE0,		// D
	0xF0, 0x80, 0xF0, 0x80, 0xF0,		// E
	0xF0, 0x80, 0xF0, 0x80, 0x80		// F
};

namespace tools::chip8 {

Chip8::Chip8() {
    reset();
    srand(time(nullptr));

    for (int i = 0; i < FONTSET_SIZE; i++) {
        _memory[i] = fontset[i];
	}
}

Chip8::~Chip8() {}

void Chip8::reset() {
    // Most chip-8 programs start at 0x200
    // because 0x0-0x1ff used to contain the interpreter.
    _pc = PROGRAM_START;

    _i = 0;
    _delay_timer = 0;
    _sound_timer = 0;

    _stack.clear();

    _vx = _v;
    _vy = _v;

    cls();
    memset(_memory, 0, MEMORY_SIZE);
    memset(_v, 0, REGISTERS_SIZE);
    memset(_keys, 0, KEYS);
}

bool Chip8::load_rom(const std::string &path) {
    if (path.empty()) {
        SPDLOG_ERROR("Cannot load file, empty path.");
        return false;
    }

    auto rom = tools::utils::files::read_binary_file(path);
    if (rom.empty()) {
        SPDLOG_ERROR("Failed to load rom file '{}'.", path);
        return false;
    }

    if (rom.size() > (uint16_t)(MEMORY_SIZE - PROGRAM_SIZE)) {
        SPDLOG_ERROR("Rom '{}' is too large to be loaded into memory. Size is {} bytes.", path, rom.size());
        return false;
    }

    for (uint16_t i = 0 ; i < rom.size() ; ++i)
        _memory[i + PROGRAM_START] = rom[i];

    return true;
}

void Chip8::log_memory(uint16_t length, uint16_t offset) {
    uint16_t l = length == 0 ? MEMORY_SIZE : length;
    std::vector to_print(_memory + offset, _memory + offset + l);
    SPDLOG_INFO(spdlog::to_hex(to_print));
}

void Chip8::dump_memory(const std::string &path) {
    if (!path.empty()) {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<char *>(_memory), MEMORY_SIZE);
    }
}

void Chip8::log_v() {
    std::vector to_print(_v, _v + REGISTERS_SIZE);
    SPDLOG_DEBUG("V = {}", spdlog::to_hex(to_print));
}

const bool *Chip8::get_screen_buffer() {
    return _screen;
}

void Chip8::next_instruction() {
    decode_execute(fetch());
}

void Chip8::decrease_timers() {
    if (_delay_timer > 0) --_delay_timer;
    if (_sound_timer > 0) --_sound_timer;
}

void Chip8::key_pressed(uint8_t key) {
    _keys[key] = 1;
}

void Chip8::key_released(uint8_t key) {
    _keys[key] = 0;
}

const uint8_t  *Chip8::get_memory() {
    return _memory;
}

const uint8_t *Chip8::get_v() {
    return _v;
}

uint16_t Chip8::get_pc() {
    return _pc;
}

uint16_t Chip8::get_i() {
    return _i;
}

std::vector<uint16_t> Chip8::get_stack() {
    return _stack;
}

uint8_t Chip8::get_delay_timer() {
    return _delay_timer;
}

uint8_t Chip8::get_sound_timer() {
    return _sound_timer;
}

const uint8_t *Chip8::get_keys() {
    return _keys;
}


int8_t Chip8::get_pressed_key() {
    for (int i = 0 ; i < KEYS ; ++i) {
        if (_keys[i]) return i;
    }
    return -1;
}

uint16_t Chip8::fetch() {
    // Memory is 8 bits but instructions are 16 bits.
    // So we assemble data from memory at _pc and _pc + 1.
    uint16_t opcode = (_memory[_pc] << 8) | _memory[_pc + 1];
    SPDLOG_DEBUG("Fetched opcode 0x{:X} at PC = 0x{:03X} (0x{:03X})", opcode, _pc, _pc - 0x200);

    // Increase pc to next instruction.
    _pc += 2;
    return opcode;
}

void Chip8::decode_execute(uint16_t opcode) {
    _msb    = (opcode >> 12) & 0x000f;
    _addr   = opcode & 0x0fff;
    _const8 = opcode & 0x00ff;
    _const4 = opcode & 0x000f;

    _x = (opcode >> 8) & 0x000f;
    _y = (opcode >> 4) & 0x000f;
    _vx = _v + _x;
    _vy = _v + _y;

    SPDLOG_DEBUG("msb = 0x{:X}\taddr = 0x{:03X}\t8 lsb = 0x{:02X}\t4 lsb = 0x{:X}\tVX = 0x{:X}\tVY = 0x{:X}", _msb, _addr, _const8, _const4, *_vx, *_vy);

    switch (_msb) {
        case 0x0: decode_op_0();    break;
        case 0x1: jump();           break;
        case 0x2: call();           break;
        case 0x3: skip_eq();        break;
        case 0x4: skip_neq();       break;
        case 0x5: skip_eq_x_y();    break;
        case 0x6: set_vx();         break;
        case 0x7: add_to_vx();      break;
        case 0x8: decode_op_8();    break;
        case 0x9: skip_neq_x_y();   break;
        case 0xa: set_i();          break;
        case 0xb: jump_v0();        break;
        case 0xc: rand_and();       break;
        case 0xd: draw();           break;
        case 0xe: decode_op_e();    break;
        case 0xf: decode_op_f();    break;
        default: break;
    }
}

void Chip8::decode_op_0() {
    switch (_const8) {
        case 0xe0: cls(); break;
        case 0xee: ret(); break;
        default: break;
    }
}

void Chip8::cls() {
    SPDLOG_DEBUG("Clear screen");
    memset(_screen, 0, SCREEN_SIZE);
}

void Chip8::ret() {
    SPDLOG_DEBUG("Return");
    _pc = _stack.back();
    _stack.pop_back();
    SPDLOG_DEBUG("PC = 0x{:X}", _pc);
}

void Chip8::jump() {
    SPDLOG_DEBUG("Jump");
    _pc = _addr;
    SPDLOG_DEBUG("PC = 0x{:X}", _pc);
}

void Chip8::call() {
    SPDLOG_DEBUG("Call");
    _stack.push_back(_pc);
    _pc = _addr;
    SPDLOG_DEBUG("PC = 0x{:X}", _pc);
}

void Chip8::skip_eq() {
    SPDLOG_DEBUG("Skip eq");
    SPDLOG_DEBUG("Comparing vx (0x{:X}) and const8 (0x{:X})", *_vx, _const8);
    if (*_vx == _const8)
        _pc += 2;
    SPDLOG_DEBUG("pc = 0x{:X}", _pc);
}

void Chip8::skip_neq() {
    SPDLOG_DEBUG("Skip neq");
    SPDLOG_DEBUG("Comparing vx (0x{:X}) and const8 (0x{:X})", *_vx, _const8);
    if (*_vx != _const8)
        _pc += 2;
    SPDLOG_DEBUG("pc = 0x{:X}", _pc);
}

void Chip8::skip_eq_x_y() {
    SPDLOG_DEBUG("Skip eq x y");
    SPDLOG_DEBUG("Comparing vx (0x{:X}) and vy (0x{:X})", *_vx, *_vy);
    if (*_vx == *_vy)
        _pc += 2;
    SPDLOG_DEBUG("pc = 0x{:X}", _pc);
}

void Chip8::set_vx() {
    SPDLOG_DEBUG("Set vx");
    *_vx = _const8;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::add_to_vx() {
    SPDLOG_DEBUG("Add to vx");
    *_vx += _const8;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::decode_op_8() {
    switch (_const4) {
        case 0x0: vx_to_vy();         break;
        case 0x1: vx_or_vy();         break;
        case 0x2: vx_and_vy();        break;
        case 0x3: vx_xor_vy();        break;
        case 0x4: add_vy_to_vx();     break;
        case 0x5: sub_vy_to_vx();     break;
        case 0x6: shift_vx_right();   break;
        case 0x7: vy_minus_vx();      break;
        case 0xe: shift_vx_left();    break;
        default: break;
    }
}

void Chip8::vx_to_vy() {
    SPDLOG_DEBUG("assign vy to vx");
    *_vx = *_vy;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::vx_or_vy() {
    SPDLOG_DEBUG("vx |= vy");
    *_vx |= *_vy;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::vx_and_vy() {
    SPDLOG_DEBUG("vx &= vy");
    *_vx &= *_vy;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::vx_xor_vy() {
    SPDLOG_DEBUG("vx ^= vy");
    *_vx ^= *_vy;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::add_vy_to_vx() {
    SPDLOG_DEBUG("add vy to vx");
    _tmp = *_vx + *_vy;
    VF = _tmp > 0xff ? 1 : 0;
    *_vx = _tmp;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::sub_vy_to_vx() {
    SPDLOG_DEBUG("sub vy to vx");
    VF = *_vy > *_vx ? 0 : 1;
    *_vx -= *_vy;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::shift_vx_right() {
    SPDLOG_DEBUG("shift vx right");
    VF = *_vx & 0x1;
    *_vx >>= 1;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::vy_minus_vx() {
    SPDLOG_DEBUG("vy - vx");
    VF = *_vx > *_vy ? 0 : 1;
    *_vx = *_vy - *_vx;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::shift_vx_left() {
    SPDLOG_DEBUG("shift vx left");
    VF = (*_vx >> 7) & 0x1;
    *_vx <<= 1;
}

void Chip8::skip_neq_x_y() {
    SPDLOG_DEBUG("skip neq x y");
    if (*_vx != *_vy) _pc += 2;
}

void Chip8::set_i() {
    SPDLOG_DEBUG("set I");
    _i = _addr;
    SPDLOG_DEBUG("I = 0x{:04X}", _i);
}

void Chip8::jump_v0() {
    SPDLOG_DEBUG("jump to VO + addr");
    _pc = V0 + _addr;
}

void Chip8::rand_and() {
    SPDLOG_DEBUG("rand and");
    _random = rand();
    *_vx = _random & _const8;

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::draw() {
    SPDLOG_DEBUG("draw");
    SPDLOG_DEBUG("Sprite is {} rows high.", _const4);
    SPDLOG_DEBUG("Drawing a total of {} pixels.", 8 * _const4);

    #ifdef DEBUG
    log_v();
    #endif

    VF = 0;

    SPDLOG_DEBUG("vx = {}, vy = {}", *_vx, *_vy);

    uint8_t mask = 0;

    // A sprite is 8 pixels wide
    // and N_const4 pixels high.

    // Each line of the sprite is stored
    // at a different address starting from _i.

    // Iterate over sprite's lines.
    for (uint8_t ysprite = 0 ; ysprite < _const4 ; ++ysprite) {
        // Each line is represented by a byte.
        uint8_t line = _memory[_i + ysprite];

        // Iterate over pixels in current sprite's line.
        for (int xsprite = 0 ; xsprite < 8 ; ++xsprite) {
            mask = 1 << (7 - xsprite);
            // If the selected bit in the line is 1
            if ((line & mask) != 0) {
                // Get pixel coords on screen.
                uint8_t x = (*_vx + xsprite) % WIDTH;
                uint8_t y = (*_vy + ysprite) % HEIGHT;
                uint16_t screen_coords = y * WIDTH + x;

                // If the pixel is set to 0, raise VF.
                if (_screen[screen_coords] == 1)
                    VF = 1;

                // Toggle the bit.
                _screen[screen_coords] ^= 1;
            }
        }
    }

    #ifdef DEBUG
    log_v();
    #endif
}

void Chip8::decode_op_e() {
    switch (_const8) {
        case 0x9e: skip_key_eq();   break;
        case 0xa1: skip_key_neq();  break;
        default: break;
    }
}

void Chip8::skip_key_eq() {
    SPDLOG_DEBUG("skip key eq");
    if (_keys[*_vx]) _pc += 2;
}

void Chip8::skip_key_neq() {
    SPDLOG_DEBUG("skpi key neq");
    if (!_keys[*_vx]) _pc += 2;
}

void Chip8::decode_op_f() {
    switch (_const8) {
        case 0x07: get_delay();         break;
        case 0x0a: get_key();           break;
        case 0x15: set_delay_timer();   break;
        case 0x18: set_sound_timer();   break;
        case 0x1e: add_to_i();          break;
        case 0x29: set_i_to_char();     break;
        case 0x33: store_decimal();     break;
        case 0x55: dump_v();            break;
        case 0x65: load_v();            break;
        default: break;
    }
}

void Chip8::get_delay() {
    SPDLOG_DEBUG("get delay timer");
    *_vx = _delay_timer;
}

void Chip8::get_key() {
    SPDLOG_DEBUG("get key");
    int8_t pressed_key = get_pressed_key();
    if (pressed_key == -1)
        _pc -= 2;
    else
        *_vx = pressed_key;
}

void Chip8::set_delay_timer() {
    SPDLOG_DEBUG("set delay timer");
    _delay_timer = *_vx;
}

void Chip8::set_sound_timer() {
    SPDLOG_DEBUG("set sound timer");
    _sound_timer = *_vx;
}

void Chip8::add_to_i() {
    SPDLOG_DEBUG("add to I");
    _i += *_vx;
    SPDLOG_DEBUG("I = 0x{:04X}", _i);
}

void Chip8::set_i_to_char() {
    SPDLOG_DEBUG("set I to char");
    _i = *_vx * 5;
}

void Chip8::store_decimal() {
    SPDLOG_DEBUG("store_decimal");
    _memory[_i] = *_vx / 100;
    _memory[_i + 1] = (*_vx / 10) % 10;
    _memory[_i + 2] = *_vx % 10;
}

void Chip8::dump_v() {
    SPDLOG_DEBUG("dump V");
    for (int i = 0 ; i <= _x ; ++i) {
        _memory[_i++] = _v[i];
        SPDLOG_DEBUG("v[{}] = {}", i, _v[i]);
    }
}

void Chip8::load_v() {
    SPDLOG_DEBUG("load V");
    for (int i = 0 ; i <= _x ; ++i) {
        SPDLOG_DEBUG("v[{}] = {}", i, _v[i]);
        _v[i] = _memory[_i++];
    }

    #ifdef DEBUG
    log_v();
    #endif
}

} // namespace tools::chip8