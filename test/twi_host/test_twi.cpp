// Pruebas de twi.c contra el modelo del I2C del STM8S.
#include "twi_model.cpp"

static int fails = 0, checks = 0;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FALLO linea %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static std::string bus() { std::string r; for (auto &x : hw.log) { r += x; r += ' '; } return r; }
static void fresh(int step = 3) {
	hw = Model(); hw.step_every = step; recover_calls = 0; ms_frozen = false;
	hw.slaves[0x50] = Slave();
	for (int i = 0; i < 256; i++) hw.slaves[0x50].mem[i] = (uint8_t)(0xA0 + i);
	twi_setTimeout(25);
	twi_init();
}

int main()
{
	for (int step : {5, 9, 17, 33}) {
		printf("== velocidad del bus: 1 evento cada %d accesos\n", step);

		// sondeo: presente / ausente
		fresh(step);
		CHECK(twi_probe(0x50) == 0, "probe 0x50 -> %d", twi_lastError());
		CHECK(bus() == "S A50 W+ P ", "bus probe: %s", bus().c_str());
		fresh(step);
		CHECK(twi_probe(0x21) == TWI_ERR_NACK_ADDR, "probe ausente");
		CHECK(bus() == "S A21 W- P ", "bus probe ausente: %s", bus().c_str());
		CHECK(twi_probe(0x50) == 0, "probe tras NACK");

		// escritura de registro + datos
		fresh(step);
		uint8_t reg = 0x10, d[5] = {1, 2, 3, 4, 5};
		CHECK(twi_writeReg(0x50, &reg, 1, d, 5, 1) == 0, "writeReg");
		CHECK(!memcmp(&hw.slaves[0x50].mem[0x10], d, 5), "datos escritos");
		CHECK(bus() == "S A50 W+ W10+ W01+ W02+ W03+ W04+ W05+ P ", "bus writeReg: %s", bus().c_str());

		// lecturas de 1, 2, 3, 4 y 10 bytes desde el registro 0x20 (inicio repetido)
		for (int n : {1, 2, 3, 4, 10, 32}) {
			fresh(step);
			uint8_t r = 0x20, buf[40]; memset(buf, 0, sizeof buf);
			CHECK(twi_writeReg(0x50, &r, 1, 0, 0, 0) == 0, "write sin stop");
			uint8_t got = twi_readFrom(0x50, buf, n, 1);
			CHECK(got == n, "read %d -> %d (err %d) bus: %s", n, got, twi_lastError(), bus().c_str());
			bool ok = true; for (int i = 0; i < n; i++) if (buf[i] != 0xA0 + 0x20 + i) ok = false;
			CHECK(ok, "datos leidos n=%d: %02X %02X %02X", n, buf[0], buf[1], buf[2]);
			CHECK(hw.slaves[0x50].bytes_read == n, "el esclavo envio %d bytes (esperados %d) bus: %s", hw.slaves[0x50].bytes_read, n, bus().c_str());
			// el ultimo con NACK, los demas con ACK, inicio repetido y STOP final
			std::string b = bus();
			CHECK(b.find("Sr") != std::string::npos && b.find(" P ") == b.size() - 3, "secuencia n=%d: %s", n, b.c_str());
			int nacks = 0; for (auto &x : hw.log) if (x[0] == 'R' && x.back() == '-') nacks++;
			CHECK(nacks == 1 && hw.log[hw.log.size() - 2].back() == '-', "NACK solo en el ultimo n=%d: %s", n, b.c_str());
			CHECK(!(hw.SR3 & SR3_BUSY), "bus libre al final n=%d", n);
		}

		// lecturas seguidas sin reiniciar (el estado POS/ACK queda bien)
		fresh(step);
		{
			uint8_t b1[2], b2[1], b3[5];
			CHECK(twi_readFrom(0x50, b2, 1, 1) == 1, "seguida 1");
			CHECK(twi_readFrom(0x50, b1, 2, 1) == 2, "seguida 2");
			CHECK(twi_readFrom(0x50, b3, 5, 1) == 5, "seguida 5");
			CHECK(b2[0] == 0xA0 && b1[0] == 0xA1 && b1[1] == 0xA2 && b3[4] == 0xA7, "datos seguidas %02X %02X %02X %02X", b2[0], b1[0], b1[1], b3[4]);
		}

		// NACK a un dato
		fresh(step);
		hw.slaves[0x50].nack_after = 2;
		{
			uint8_t r = 0, dd[4] = {9, 9, 9, 9};
			CHECK(twi_writeReg(0x50, &r, 1, dd, 4, 1) == TWI_ERR_NACK_DATA, "NACK de dato -> %d bus: %s", twi_lastError(), bus().c_str());
			CHECK(!(hw.SR3 & SR3_BUSY), "bus libre tras NACK de dato: %s", bus().c_str());
			CHECK(twi_probe(0x50) == 0, "funciona tras NACK de dato");
		}
	}

	// bus bloqueado (BUSY siempre a 1): tiempo agotado y recuperacion
	printf("== bus bloqueado\n");
	fresh(3);
	hw.stuck_busy = true; hw.SR3 |= SR3_BUSY;
	CHECK(twi_probe(0x50) == TWI_ERR_TIMEOUT, "timeout con bus ocupado");
	CHECK(recover_calls == 1, "recuperacion llamada %d", recover_calls);

	// sin que millis() avance (p. ej. dentro de una interrupcion): el tope de vueltas corta
	printf("== millis() detenido\n");
	fresh(3);
	hw.stuck_busy = true; hw.SR3 |= SR3_BUSY; ms_frozen = true;
	CHECK(twi_probe(0x50) == TWI_ERR_TIMEOUT, "timeout sin millis");

	// tiempo de espera 0 = sin limite: no aplica el tope (no se prueba el cuelgue)

	// frecuencias
	printf("== frecuencias\n");
	fresh(3);
	twi_setFrequency(100000); CHECK(hw.CCRL == 80 && hw.CCRH == 0 && hw.TRISER == 17, "100k CCR=%d TRISE=%d", hw.CCRL, hw.TRISER);
	twi_setFrequency(400000); CHECK(hw.CCRL == 14 && hw.CCRH == 0x80 && hw.TRISER == 5, "400k CCR=%d CCRH=%02X TRISE=%d", hw.CCRL, hw.CCRH, hw.TRISER);
	twi_setFrequency(50000);  CHECK(hw.CCRL == 160 && hw.CCRH == 0, "50k CCR=%d", hw.CCRL);
	twi_setFrequency(10000);  CHECK(hw.CCRL == (800 & 0xFF) && hw.CCRH == 3, "10k CCR=%d CCRH=%d", hw.CCRL, hw.CCRH);

	// operaciones al azar con un bus de ritmo irregular
	printf("== 3000 operaciones al azar\n");
	srand(1234);
	fresh(3); hw.jitter = 40;
	int bad = 0;
	for (int i = 0; i < 3000; i++) {
		int n = 1 + rand() % 32;
		uint8_t reg = rand() & 0xFF, out[32], in[32];
		for (int k = 0; k < n; k++) out[k] = rand();
		hw.log.clear();
		if (twi_writeReg(0x50, &reg, 1, out, n, 1)) { bad++; printf("  wr err %d\n", twi_lastError()); continue; }
		if (twi_writeReg(0x50, &reg, 1, 0, 0, 0)) { bad++; continue; }
		hw.slaves[0x50].bytes_read = 0;
		if (twi_readFrom(0x50, in, n, 1) != n) { bad++; continue; }
		uint8_t exp[32]; for (int k = 0; k < n; k++) exp[k] = hw.slaves[0x50].mem[(uint8_t)(reg + k)];
		if (memcmp(in, out, n) || memcmp(in, exp, n) || hw.slaves[0x50].bytes_read != n) {
			bad++;
			if (bad <= 3) {
				printf("  op %d n=%d reg=%02X leidos=%d\n   esperado:", i, n, reg, hw.slaves[0x50].bytes_read);
				for (int k = 0; k < n; k++) printf(" %02X", out[k]);
				printf("\n   obtenido:"); for (int k = 0; k < n; k++) printf(" %02X", in[k]);
				size_t from = hw.log.size() > 50 ? hw.log.size() - 50 : 0; printf("\n   bus:");
				for (size_t k = from; k < hw.log.size(); k++) printf(" %s", hw.log[k].c_str());
				printf("\n");
			}
		}
		if (rand() % 10 == 0) { uint8_t b; twi_readFrom(0x50, &b, 1, 1); }
		if (rand() % 15 == 0) twi_probe(0x33);
	}
	CHECK(bad == 0, "%d operaciones al azar fallaron", bad);

	printf("\n%d comprobaciones, %d fallos\n", checks, fails);
	return fails ? 1 : 0;
}
