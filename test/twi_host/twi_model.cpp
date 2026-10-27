// Modelo de comportamiento del I2C del STM8S (RM0016) para probar twi.c en el PC.
//
// El bus avanza un "evento" (un byte, un START...) cada pocos accesos a
// registros: la CPU es mucho mas rapida que el bus, como en el hardware.
// Los esclavos simulados registran lo que ven para comprobar la secuencia
// (ACK/NACK, STOP, cuantos bytes se les pidieron).

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>

enum { SR1_SB = 0x01, SR1_ADDR = 0x02, SR1_BTF = 0x04, SR1_RXNE = 0x40, SR1_TXE = 0x80 };
enum { SR2_BERR = 0x01, SR2_ARLO = 0x02, SR2_AF = 0x04 };
enum { SR3_MSL = 0x01, SR3_BUSY = 0x02, SR3_TRA = 0x04 };
enum { CR2_START = 0x01, CR2_STOP = 0x02, CR2_ACK = 0x04, CR2_POS = 0x08, CR2_SWRST = 0x80 };

struct Slave {
	uint8_t mem[256] = {0};
	uint8_t ptr = 0;
	bool first = true;           // primer byte de una escritura = puntero de registro
	int nack_after = -1;         // NACK a partir del dato n de una escritura
	int written = 0;
	int bytes_read = 0;          // bytes que el maestro le pidio
};

struct Model {
	uint8_t CR1 = 0, CR2 = 0, FREQR = 0, CCRL = 0, CCRH = 0, TRISER = 0;
	uint8_t SR1 = 0, SR2 = 0, SR3 = 0;
	uint8_t DR_rx = 0, DR_tx = 0, tx_shift = 0, rx_shift = 0;
	bool dr_tx_full = false, shift_busy = false;
	bool sr1_read_with_sb = false, sr1_read_with_addr = false, sr1_read_with_btf = false;
	int phase = 0;               // 0 libre, 1 START enviado, 2 transmisor, 3 receptor
	int cur_addr = -1;           // -1: direccion con NACK
	bool rx_inprog = false;      // byte recibiendose en el registro de desplazamiento
	bool tx_nacked = false;      // el esclavo respondio NACK a un dato
	bool rx_stalled = false;     // BTF en recepcion
	bool rx_done = false;        // ya se envio NACK
	int pos_next_ack = 1;        // con POS=1: ACK decidido para el siguiente byte
	bool stuck_busy = false;
	int accesses = 0;
	int step_every = 3;
	std::map<int, Slave> slaves;
	std::vector<std::string> log;

	void L(const char *fmt, int a = 0) { char t[64]; snprintf(t, sizeof t, fmt, a); log.push_back(t); }

	void reset_bus() {
		phase = 0; cur_addr = -1;
		SR3 &= ~(SR3_MSL | SR3_TRA);
		if (!stuck_busy) SR3 &= ~SR3_BUSY;
		shift_busy = dr_tx_full = rx_inprog = rx_stalled = rx_done = tx_nacked = false;
	}
	void do_stop() {
		L("P"); CR2 &= ~CR2_STOP;
		SR1 &= (uint8_t)(SR1_RXNE | SR1_BTF);   // los datos recibidos siguen en DR
		phase = 0; cur_addr = -1; SR3 &= ~(SR3_MSL | SR3_TRA);
		if (!stuck_busy) SR3 &= ~SR3_BUSY;
		shift_busy = dr_tx_full = rx_inprog = rx_done = tx_nacked = false;
	}

	void step() {
		if (!(CR1 & 1)) return;
		if ((CR2 & CR2_START) && !(SR1 & SR1_SB)) {
			bool can = !(SR3 & SR3_BUSY) || ((SR3 & SR3_MSL) && !shift_busy && !rx_inprog && !dr_tx_full);
			if (stuck_busy) can = false;
			if (can) {
				L(phase ? "Sr" : "S");
				CR2 &= ~CR2_START;
				SR1 = SR1_SB; SR3 |= SR3_MSL | SR3_BUSY; phase = 1;
				dr_tx_full = shift_busy = rx_inprog = rx_stalled = rx_done = tx_nacked = false;
				SR1 = SR1_SB;
			}
			return;
		}
		if (phase == 2) {
			if (cur_addr < 0 || tx_nacked) { if (CR2 & CR2_STOP) do_stop(); return; }
			if (dr_tx_full && !shift_busy) {
				tx_shift = DR_tx; shift_busy = true; dr_tx_full = false; SR1 |= SR1_TXE;
				return;
			}
			if (shift_busy) {
				Slave &s = slaves[cur_addr];
				bool ack = !(s.nack_after >= 0 && !s.first && s.written >= s.nack_after);
				if (s.first) { s.ptr = tx_shift; s.first = false; } else { s.mem[s.ptr++] = tx_shift; s.written++; }
				L(ack ? "W%02X+" : "W%02X-", tx_shift);
				shift_busy = false;
				if (!ack) { SR2 |= SR2_AF; tx_nacked = true; return; }
				if (!dr_tx_full) SR1 |= SR1_BTF;
				return;
			}
			if ((CR2 & CR2_STOP) && !dr_tx_full) { do_stop(); return; }
			return;
		}
		if (phase == 3) {
			if (SR1 & SR1_ADDR) return;           // reloj retenido hasta borrar ADDR
			if (rx_inprog) {                      // fin del byte: pulso ACK/NACK
				if (accesses - rx_start < min_byte) return;   // un byte dura 9 ciclos de SCL
				int ack;
				if (CR2 & CR2_POS) { ack = pos_next_ack; pos_next_ack = (CR2 & CR2_ACK) ? 1 : 0; }
				else ack = (CR2 & CR2_ACK) ? 1 : 0;
				L(ack ? "R%02X+" : "R%02X-", rx_shift);
				if (!ack) rx_done = true;
				rx_inprog = false;
				if (SR1 & SR1_RXNE) { rx_stalled = true; SR1 |= SR1_BTF; }   // DR lleno: BTF, reloj retenido
				else { DR_rx = rx_shift; SR1 |= SR1_RXNE; start_rx(); }      // tras un ACK el esclavo sigue
				return;
			}
			if (CR2 & CR2_STOP) { do_stop(); return; }
			if (!rx_stalled) start_rx();          // primer byte tras borrar ADDR
			return;
		}
	}

	void start_rx() {
		if (rx_done || rx_inprog || phase != 3 || cur_addr < 0) return;
		Slave &s = slaves[cur_addr];
		rx_shift = s.mem[s.ptr++]; s.bytes_read++;
		rx_inprog = true; rx_start = accesses;
	}

	int rx_start = 0, min_byte = 12;   // minimo de accesos que dura un byte (en el chip, ~40)
	int jitter = 0;              // >0: el intervalo entre eventos varia al azar
	int next_step = 3;
	void tick() {
		++accesses;
		if (!jitter) { if (accesses % step_every == 0) step(); return; }
		if (--next_step <= 0) { step(); next_step = 3 + rand() % jitter; }
	}

	uint8_t rd(const char *r) {
		uint8_t v = 0;
		if (!strcmp(r, "SR1")) {
			v = SR1;
			sr1_read_with_sb = SR1 & SR1_SB;
			sr1_read_with_addr = SR1 & SR1_ADDR;
			sr1_read_with_btf = SR1 & SR1_BTF;
		} else if (!strcmp(r, "SR2")) v = SR2;
		else if (!strcmp(r, "SR3")) {
			v = SR3;
			if (sr1_read_with_addr && (SR1 & SR1_ADDR)) {
				SR1 &= ~SR1_ADDR;
				if (phase == 2) SR1 |= SR1_TXE;
				if (phase == 3) { pos_next_ack = (CR2 & CR2_ACK) ? 1 : 0; start_rx(); }   // el esclavo empieza a enviar
			}
		} else if (!strcmp(r, "DR")) {
			v = DR_rx;
			if (SR1 & SR1_RXNE) {
				SR1 &= ~SR1_RXNE;
				if (rx_stalled) {             // el byte retenido pasa a DR y el bus sigue
					DR_rx = rx_shift; SR1 |= SR1_RXNE; SR1 &= ~SR1_BTF; rx_stalled = false;
					if (!(CR2 & CR2_STOP) || true) start_rx();
				}
			}
		} else if (!strcmp(r, "CR1")) v = CR1;
		else if (!strcmp(r, "CR2")) v = CR2;
		tick();
		return v;
	}

	void wr(const char *r, uint8_t v) {
		if (!strcmp(r, "DR")) {
			if (phase == 1 && sr1_read_with_sb && (SR1 & SR1_SB)) {
				SR1 &= ~SR1_SB;
				int a = v >> 1;
				bool isrd = v & 1;
				if (slaves.count(a)) {
					L(isrd ? "A%02X R+" : "A%02X W+", a);
					cur_addr = a; slaves[a].first = !isrd;
					SR1 |= SR1_ADDR;
					phase = isrd ? 3 : 2;
					if (!isrd) SR3 |= SR3_TRA; else SR3 &= ~SR3_TRA;
				} else {
					L(isrd ? "A%02X R-" : "A%02X W-", a);
					SR2 |= SR2_AF; phase = 2; cur_addr = -1;
				}
			} else if (phase == 2 && cur_addr >= 0) {
				if (!dr_tx_full) {
					DR_tx = v; dr_tx_full = true; SR1 &= ~SR1_TXE;
					if (sr1_read_with_btf) SR1 &= ~SR1_BTF;
				} else L("DR-SOBRESCRITO");
			} else L("DR-ESCRITO-FUERA-DE-TIEMPO");
		} else if (!strcmp(r, "CR1")) { CR1 = v; if (!(v & 1)) SR1 = 0; }
		else if (!strcmp(r, "CR2")) {
			if (v & CR2_SWRST) { CR1 = 0; SR1 = SR2 = 0; SR3 = stuck_busy ? SR3_BUSY : 0; reset_bus(); log.push_back("SWRST"); }
			CR2 = v & ~CR2_SWRST;
		}
		else if (!strcmp(r, "SR2")) SR2 = v;
		else if (!strcmp(r, "FREQR")) FREQR = v;
		else if (!strcmp(r, "CCRL")) CCRL = v;
		else if (!strcmp(r, "CCRH")) CCRH = v;
		else if (!strcmp(r, "TRISER")) TRISER = v;
		tick();
	}
};

Model hw;

struct Reg {
	const char *name;
	operator uint8_t() const { return hw.rd(name); }
	Reg &operator=(uint8_t v) { hw.wr(name, v); return *this; }
	Reg &operator|=(uint8_t v) { uint8_t t = hw.rd(name); hw.wr(name, t | v); return *this; }
	Reg &operator&=(uint8_t v) { uint8_t t = hw.rd(name); hw.wr(name, t & v); return *this; }
};
struct Regs {
	Reg CR1{"CR1"}, CR2{"CR2"}, FREQR{"FREQR"}, CCRL{"CCRL"}, CCRH{"CCRH"}, TRISER{"TRISER"};
	Reg SR1{"SR1"}, SR2{"SR2"}, SR3{"SR3"}, DR{"DR"};
} I2Cr;

// ---- entorno de twi.c ----
static uint32_t ms_now = 0;
static bool ms_frozen = false;
uint32_t millis(void) { if (!ms_frozen && hw.accesses % 40 == 0) ms_now++; return ms_now; }
int recover_calls = 0;
void twi_host_recover(void) { recover_calls++; hw.log.push_back("RECOVER"); }

#define TWI_HOST_TEST 1
#define F_CPU 16000000UL
#define TWI_REG(r) (I2Cr.r)
#define TWI_CRITICAL_BEGIN {
#define TWI_CRITICAL_END }
#define I2C_SR1_SB 0x01
#define I2C_SR1_ADDR 0x02
#define I2C_SR1_BTF 0x04
#define I2C_SR1_RXNE 0x40
#define I2C_SR1_TXE 0x80
#define I2C_SR2_BERR 0x01
#define I2C_SR2_ARLO 0x02
#define I2C_SR2_AF 0x04
#define I2C_SR3_BUSY 0x02
#define I2C_CR2_START 0x01
#define I2C_CR2_STOP 0x02
#define I2C_CR2_ACK 0x04
#define I2C_CR2_POS 0x08
#define I2C_CR2_SWRST 0x80
#define I2C_CR1_PE 0x01
#define I2C_CCRH_FS 0x80
#include "../../sduino/stm8/cores/sduino/twi.c"
#include "../../sduino/stm8/cores/sduino/twi_freq.c"
