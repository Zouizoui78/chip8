#ifndef CHIP8_HPP
#define CHIP8_HPP

#include <cstdint>
#include <string>
#include <vector>

#define MEMORY_SIZE 0xfff
#define PROGRAM_START 0x200
#define PROGRAM_SIZE MEMORY_SIZE-PROGRAM_START

#define REGISTERS_SIZE 16
#define KEYS 16

// Graphics sizes
#define WIDTH 64
#define HEIGHT 32
#define SCREEN_SIZE WIDTH * HEIGHT

namespace tools::chip8 {

class Chip8 {
    public:

    Chip8();

    ~Chip8();

    void reset();
    bool load_rom(const std::string &path);
    void log_memory(uint16_t length = 0, uint16_t offset = 0);
    void dump_memory(const std::string &path);

    void log_v();

    const bool *get_screen_buffer();
    uint8_t get_sound_timer();

    void next_instruction();
    void decrease_timers();

    void key_pressed(uint8_t key);
    void key_released(uint8_t key);


    protected:

    const uint8_t *get_memory();
    const uint8_t *get_v();
    uint16_t get_pc();
    uint16_t get_i();
    std::vector<uint16_t> get_stack();
    uint8_t get_delay_timer();
    const uint8_t *get_keys();

    int8_t get_pressed_key();

    uint16_t fetch();
    void decode_execute(uint16_t opcode);


    private:

    // Opcodes implementations.

    // opcode 0x0xxx
    void decode_op_0();
    void cls();
    void ret();
    ////////////////

    void jump();
    void call();

    // Skip next instruction if V[x] == value.
    void skip_eq();

    // Skip next instruction if V[x] != value.
    void skip_neq();

    // Skip next instruction if V[x] == V[y].
    void skip_eq_x_y();

    void set_vx();
    void add_to_vx();

    // opcodes 0x8xxx
    void decode_op_8();
    void vx_to_vy();
    void vx_or_vy();
    void vx_and_vy();
    void vx_xor_vy();
    void add_vy_to_vx();
    void sub_vy_to_vx();
    void shift_vx_right();
    void vy_minus_vx();
    void shift_vx_left();
    /////////////////

    void skip_neq_x_y();
    void set_i();
    void jump_v0();
    void rand_and();
    void draw();

    // opcodes 0xexxx
    void decode_op_e();
    void skip_key_eq();
    void skip_key_neq();
    /////////////////

    // opcodes 0xfxxx
    void decode_op_f();
    void get_delay();
    void get_key();
    void set_delay_timer();
    void set_sound_timer();
    void add_to_i();
    void set_i_to_char();
    void store_decimal();
    void dump_v();
    void load_v();
    /////////////////

    // 0xfff (4095) bytes of RAM.
    uint8_t _memory[MEMORY_SIZE];

    // Buffer holding screen data.
    bool _screen[SCREEN_SIZE];

    // CPU registers, named V0 to VF.
    // VF is used in some operations as a carry flag for example.
    uint8_t _v[REGISTERS_SIZE];

    // Program counter.
    uint16_t _pc;

    // Used by several opcodes doing memory operations.
    uint16_t _i;

    // For subroutines returns.
    std::vector<uint16_t> _stack;

    // Delay timer.
    uint8_t _delay_timer;
    uint8_t _sound_timer;

    // Keys.
    uint8_t _keys[KEYS];

    // Used for random number generation.
    uint8_t _random;

    // Used as a buffer in some operations.
    uint16_t _tmp;


    // opcode details
    uint8_t _msb; // most significant 4 bits of the opcode.
    uint16_t _addr; // 12 less significant bits of the opcode ; used for addresses.
    uint8_t _const8; // 8 less significant bits of the opcode
    uint8_t _const4; // 4 less significant bits of the opcode
    uint8_t _x;
    uint8_t _y;
    uint8_t *_vx; // v register pointed by the 0x0f00 bits of the opcode
    uint8_t *_vy; // v register pointed by the 0x00f0 bits of the opcode
};

} // namespace tools::chip8

#endif // CHIP8