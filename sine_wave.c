#include "global.h"
#include "sine_wave.h"
#include "utils.h"

typedef struct _sine_state_t {
	// configuration values
	uint16_t amplitude;
	uint16_t periodMs;
} sine_state_t;

static osMailQDef(sine_cfg_q, 0x8, waveform_cfg_t);
osMailQId Q_sine_cfg_id;

static osMutexDef(sine_state_m);
osMutexId M_sine_state;

static void sine_run(void const *arg);

static osTimerDef(sine_run_timer, sine_run);
static osTimerId TMR_sine_run_timer;

void sine_wave_init(void)
{
	Q_sine_cfg_id = osMailCreate(osMailQ(sine_cfg_q), NULL);
	M_sine_state = osMutexCreate(osMutex(sine_state_m));
}

void sine_wave_thread(void const *arg)
{
	sine_state_t state = {
		.amplitude = SCALE_AMPLITUDE(100),
		.periodMs = 100,
	};

	TMR_sine_run_timer = osTimerCreate(osTimer(sine_run_timer), osTimerPeriodic, &state);
	uint8_t bRunning = 0;

	osEvent retval;
	while (1) {
		retval = osMailGet(Q_sine_cfg_id, osWaitForever);
		if (retval.status == osEventMail) {
			waveform_cfg_t *cfg = retval.value.p;

			osMutexWait(M_sine_state, osWaitForever);
			switch (cfg->type) {
				case PARAM_AMPLITUDE:
				{
					state.amplitude = SCALE_AMPLITUDE(cfg->value);
					break;
				}
				case PARAM_PERIOD_MS:
				{
					state.periodMs = cfg->value;
					break;
				}
				case PARAM_ENABLE:
				{
					if (cfg->value) {
						bRunning = !bRunning;
					} else {
						bRunning = 0;
					}

					if (bRunning) {
						osTimerStart(TMR_sine_run_timer, 1);
					} else {
						osTimerStop(TMR_sine_run_timer);
						GPIO_Write(WAVEFORM_PORT, 0);
					}
					break;
				}
				default:
					break;
			}
			osMutexRelease(M_sine_state);

			osMailFree(Q_sine_cfg_id, cfg);
		}
	}
}

#define SINE_LOOKUP_SZ 1000
static uint16_t const sine_lookup[SINE_LOOKUP_SZ] = {
	0x8000,0x80cd,0x819b,0x8269,0x8337,0x8405,0x84d3,0x85a0,
	0x866e,0x873b,0x8809,0x88d6,0x89a4,0x8a71,0x8b3e,0x8c0b,
	0x8cd8,0x8da5,0x8e72,0x8f3e,0x900a,0x90d7,0x91a3,0x926e,
	0x933a,0x9405,0x94d1,0x959c,0x9667,0x9731,0x97fc,0x98c6,
	0x998f,0x9a59,0x9b22,0x9bec,0x9cb4,0x9d7d,0x9e45,0x9f0d,
	0x9fd4,0xa09c,0xa163,0xa229,0xa2ef,0xa3b5,0xa47b,0xa540,
	0xa605,0xa6c9,0xa78d,0xa851,0xa914,0xa9d7,0xaa99,0xab5b,
	0xac1d,0xacde,0xad9e,0xae5e,0xaf1e,0xafdd,0xb09c,0xb15a,
	0xb218,0xb2d5,0xb392,0xb44e,0xb50a,0xb5c5,0xb67f,0xb739,
	0xb7f3,0xb8ac,0xb964,0xba1c,0xbad3,0xbb89,0xbc3f,0xbcf5,
	0xbda9,0xbe5d,0xbf11,0xbfc4,0xc076,0xc128,0xc1d8,0xc289,
	0xc338,0xc3e7,0xc495,0xc543,0xc5ef,0xc69c,0xc747,0xc7f2,
	0xc89b,0xc945,0xc9ed,0xca95,0xcb3c,0xcbe2,0xcc87,0xcd2c,
	0xcdd0,0xce73,0xcf15,0xcfb7,0xd057,0xd0f7,0xd196,0xd235,
	0xd2d2,0xd36e,0xd40a,0xd4a5,0xd53f,0xd5d8,0xd670,0xd708,
	0xd79e,0xd834,0xd8c9,0xd95d,0xd9f0,0xda82,0xdb13,0xdba3,
	0xdc32,0xdcc1,0xdd4e,0xddda,0xde66,0xdef1,0xdf7a,0xe003,
	0xe08a,0xe111,0xe197,0xe21c,0xe29f,0xe322,0xe3a4,0xe424,
	0xe4a4,0xe523,0xe5a1,0xe61d,0xe699,0xe713,0xe78d,0xe805,
	0xe87d,0xe8f3,0xe969,0xe9dd,0xea50,0xeac2,0xeb33,0xeba3,
	0xec12,0xec80,0xecec,0xed58,0xedc3,0xee2c,0xee94,0xeefb,
	0xef61,0xefc6,0xf02a,0xf08c,0xf0ee,0xf14e,0xf1ae,0xf20c,
	0xf268,0xf2c4,0xf31f,0xf378,0xf3d0,0xf427,0xf47d,0xf4d2,
	0xf526,0xf578,0xf5c9,0xf619,0xf668,0xf6b6,0xf702,0xf74d,
	0xf797,0xf7e0,0xf827,0xf86e,0xf8b3,0xf8f7,0xf93a,0xf97b,
	0xf9bb,0xf9fa,0xfa38,0xfa75,0xfab0,0xfaea,0xfb23,0xfb5a,
	0xfb91,0xfbc6,0xfbfa,0xfc2c,0xfc5d,0xfc8e,0xfcbc,0xfcea,
	0xfd16,0xfd41,0xfd6b,0xfd93,0xfdbb,0xfde1,0xfe05,0xfe29,
	0xfe4b,0xfe6c,0xfe8b,0xfea9,0xfec6,0xfee2,0xfefd,0xff16,
	0xff2e,0xff44,0xff5a,0xff6e,0xff80,0xff92,0xffa2,0xffb1,
	0xffbe,0xffcb,0xffd6,0xffdf,0xffe8,0xffef,0xfff5,0xfff9,
	0xfffc,0xfffe,0xffff,0xfffe,0xfffc,0xfff9,0xfff5,0xffef,
	0xffe8,0xffdf,0xffd6,0xffcb,0xffbe,0xffb1,0xffa2,0xff92,
	0xff80,0xff6e,0xff5a,0xff44,0xff2e,0xff16,0xfefd,0xfee2,
	0xfec6,0xfea9,0xfe8b,0xfe6c,0xfe4b,0xfe29,0xfe05,0xfde1,
	0xfdbb,0xfd93,0xfd6b,0xfd41,0xfd16,0xfcea,0xfcbc,0xfc8e,
	0xfc5d,0xfc2c,0xfbfa,0xfbc6,0xfb91,0xfb5a,0xfb23,0xfaea,
	0xfab0,0xfa75,0xfa38,0xf9fa,0xf9bb,0xf97b,0xf93a,0xf8f7,
	0xf8b3,0xf86e,0xf827,0xf7e0,0xf797,0xf74d,0xf702,0xf6b6,
	0xf668,0xf619,0xf5c9,0xf578,0xf526,0xf4d2,0xf47d,0xf427,
	0xf3d0,0xf378,0xf31f,0xf2c4,0xf268,0xf20c,0xf1ae,0xf14e,
	0xf0ee,0xf08c,0xf02a,0xefc6,0xef61,0xeefb,0xee94,0xee2c,
	0xedc3,0xed58,0xecec,0xec80,0xec12,0xeba3,0xeb33,0xeac2,
	0xea50,0xe9dd,0xe969,0xe8f3,0xe87d,0xe805,0xe78d,0xe713,
	0xe699,0xe61d,0xe5a1,0xe523,0xe4a4,0xe424,0xe3a4,0xe322,
	0xe29f,0xe21c,0xe197,0xe111,0xe08a,0xe003,0xdf7a,0xdef1,
	0xde66,0xddda,0xdd4e,0xdcc1,0xdc32,0xdba3,0xdb13,0xda82,
	0xd9f0,0xd95d,0xd8c9,0xd834,0xd79e,0xd708,0xd670,0xd5d8,
	0xd53f,0xd4a5,0xd40a,0xd36e,0xd2d2,0xd235,0xd196,0xd0f7,
	0xd057,0xcfb7,0xcf15,0xce73,0xcdd0,0xcd2c,0xcc87,0xcbe2,
	0xcb3c,0xca95,0xc9ed,0xc945,0xc89b,0xc7f2,0xc747,0xc69c,
	0xc5ef,0xc543,0xc495,0xc3e7,0xc338,0xc289,0xc1d8,0xc128,
	0xc076,0xbfc4,0xbf11,0xbe5d,0xbda9,0xbcf5,0xbc3f,0xbb89,
	0xbad3,0xba1c,0xb964,0xb8ac,0xb7f3,0xb739,0xb67f,0xb5c5,
	0xb50a,0xb44e,0xb392,0xb2d5,0xb218,0xb15a,0xb09c,0xafdd,
	0xaf1e,0xae5e,0xad9e,0xacde,0xac1d,0xab5b,0xaa99,0xa9d7,
	0xa914,0xa851,0xa78d,0xa6c9,0xa605,0xa540,0xa47b,0xa3b5,
	0xa2ef,0xa229,0xa163,0xa09c,0x9fd4,0x9f0d,0x9e45,0x9d7d,
	0x9cb4,0x9bec,0x9b22,0x9a59,0x998f,0x98c6,0x97fc,0x9731,
	0x9667,0x959c,0x94d1,0x9405,0x933a,0x926e,0x91a3,0x90d7,
	0x900a,0x8f3e,0x8e72,0x8da5,0x8cd8,0x8c0b,0x8b3e,0x8a71,
	0x89a4,0x88d6,0x8809,0x873b,0x866e,0x85a0,0x84d3,0x8405,
	0x8337,0x8269,0x819b,0x80cd,0x8000,0x7f32,0x7e64,0x7d96,
	0x7cc8,0x7bfa,0x7b2c,0x7a5f,0x7991,0x78c4,0x77f6,0x7729,
	0x765b,0x758e,0x74c1,0x73f4,0x7327,0x725a,0x718d,0x70c1,
	0x6ff5,0x6f28,0x6e5c,0x6d91,0x6cc5,0x6bfa,0x6b2e,0x6a63,
	0x6998,0x68ce,0x6803,0x6739,0x6670,0x65a6,0x64dd,0x6413,
	0x634b,0x6282,0x61ba,0x60f2,0x602b,0x5f63,0x5e9c,0x5dd6,
	0x5d10,0x5c4a,0x5b84,0x5abf,0x59fa,0x5936,0x5872,0x57ae,
	0x56eb,0x5628,0x5566,0x54a4,0x53e2,0x5321,0x5261,0x51a1,
	0x50e1,0x5022,0x4f63,0x4ea5,0x4de7,0x4d2a,0x4c6d,0x4bb1,
	0x4af5,0x4a3a,0x4980,0x48c6,0x480c,0x4753,0x469b,0x45e3,
	0x452c,0x4476,0x43c0,0x430a,0x4256,0x41a2,0x40ee,0x403b,
	0x3f89,0x3ed7,0x3e27,0x3d76,0x3cc7,0x3c18,0x3b6a,0x3abc,
	0x3a10,0x3963,0x38b8,0x380d,0x3764,0x36ba,0x3612,0x356a,
	0x34c3,0x341d,0x3378,0x32d3,0x322f,0x318c,0x30ea,0x3048,
	0x2fa8,0x2f08,0x2e69,0x2dca,0x2d2d,0x2c91,0x2bf5,0x2b5a,
	0x2ac0,0x2a27,0x298f,0x28f7,0x2861,0x27cb,0x2736,0x26a2,
	0x260f,0x257d,0x24ec,0x245c,0x23cd,0x233e,0x22b1,0x2225,
	0x2199,0x210e,0x2085,0x1ffc,0x1f75,0x1eee,0x1e68,0x1de3,
	0x1d60,0x1cdd,0x1c5b,0x1bdb,0x1b5b,0x1adc,0x1a5e,0x19e2,
	0x1966,0x18ec,0x1872,0x17fa,0x1782,0x170c,0x1696,0x1622,
	0x15af,0x153d,0x14cc,0x145c,0x13ed,0x137f,0x1313,0x12a7,
	0x123c,0x11d3,0x116b,0x1104,0x109e,0x1039,0x0fd5,0x0f73,
	0x0f11,0x0eb1,0x0e51,0x0df3,0x0d97,0x0d3b,0x0ce0,0x0c87,
	0x0c2f,0x0bd8,0x0b82,0x0b2d,0x0ad9,0x0a87,0x0a36,0x09e6,
	0x0997,0x0949,0x08fd,0x08b2,0x0868,0x081f,0x07d8,0x0791,
	0x074c,0x0708,0x06c5,0x0684,0x0644,0x0605,0x05c7,0x058a,
	0x054f,0x0515,0x04dc,0x04a5,0x046e,0x0439,0x0405,0x03d3,
	0x03a2,0x0371,0x0343,0x0315,0x02e9,0x02be,0x0294,0x026c,
	0x0244,0x021e,0x01fa,0x01d6,0x01b4,0x0193,0x0174,0x0156,
	0x0139,0x011d,0x0102,0x00e9,0x00d1,0x00bb,0x00a5,0x0091,
	0x007f,0x006d,0x005d,0x004e,0x0041,0x0034,0x0029,0x0020,
	0x0017,0x0010,0x000a,0x0006,0x0003,0x0001,0x0000,0x0001,
	0x0003,0x0006,0x000a,0x0010,0x0017,0x0020,0x0029,0x0034,
	0x0041,0x004e,0x005d,0x006d,0x007f,0x0091,0x00a5,0x00bb,
	0x00d1,0x00e9,0x0102,0x011d,0x0139,0x0156,0x0174,0x0193,
	0x01b4,0x01d6,0x01fa,0x021e,0x0244,0x026c,0x0294,0x02be,
	0x02e9,0x0315,0x0343,0x0371,0x03a2,0x03d3,0x0405,0x0439,
	0x046e,0x04a5,0x04dc,0x0515,0x054f,0x058a,0x05c7,0x0605,
	0x0644,0x0684,0x06c5,0x0708,0x074c,0x0791,0x07d8,0x081f,
	0x0868,0x08b2,0x08fd,0x0949,0x0997,0x09e6,0x0a36,0x0a87,
	0x0ad9,0x0b2d,0x0b82,0x0bd8,0x0c2f,0x0c87,0x0ce0,0x0d3b,
	0x0d97,0x0df3,0x0e51,0x0eb1,0x0f11,0x0f73,0x0fd5,0x1039,
	0x109e,0x1104,0x116b,0x11d3,0x123c,0x12a7,0x1313,0x137f,
	0x13ed,0x145c,0x14cc,0x153d,0x15af,0x1622,0x1696,0x170c,
	0x1782,0x17fa,0x1872,0x18ec,0x1966,0x19e2,0x1a5e,0x1adc,
	0x1b5b,0x1bdb,0x1c5b,0x1cdd,0x1d60,0x1de3,0x1e68,0x1eee,
	0x1f75,0x1ffc,0x2085,0x210e,0x2199,0x2225,0x22b1,0x233e,
	0x23cd,0x245c,0x24ec,0x257d,0x260f,0x26a2,0x2736,0x27cb,
	0x2861,0x28f7,0x298f,0x2a27,0x2ac0,0x2b5a,0x2bf5,0x2c91,
	0x2d2d,0x2dca,0x2e69,0x2f08,0x2fa8,0x3048,0x30ea,0x318c,
	0x322f,0x32d3,0x3378,0x341d,0x34c3,0x356a,0x3612,0x36ba,
	0x3764,0x380d,0x38b8,0x3963,0x3a10,0x3abc,0x3b6a,0x3c18,
	0x3cc7,0x3d76,0x3e27,0x3ed7,0x3f89,0x403b,0x40ee,0x41a2,
	0x4256,0x430a,0x43c0,0x4476,0x452c,0x45e3,0x469b,0x4753,
	0x480c,0x48c6,0x4980,0x4a3a,0x4af5,0x4bb1,0x4c6d,0x4d2a,
	0x4de7,0x4ea5,0x4f63,0x5022,0x50e1,0x51a1,0x5261,0x5321,
	0x53e2,0x54a4,0x5566,0x5628,0x56eb,0x57ae,0x5872,0x5936,
	0x59fa,0x5abf,0x5b84,0x5c4a,0x5d10,0x5dd6,0x5e9c,0x5f63,
	0x602b,0x60f2,0x61ba,0x6282,0x634b,0x6413,0x64dd,0x65a6,
	0x6670,0x6739,0x6803,0x68ce,0x6998,0x6a63,0x6b2e,0x6bfa,
	0x6cc5,0x6d91,0x6e5c,0x6f28,0x6ff5,0x70c1,0x718d,0x725a,
	0x7327,0x73f4,0x74c1,0x758e,0x765b,0x7729,0x77f6,0x78c4,
	0x7991,0x7a5f,0x7b2c,0x7bfa,0x7cc8,0x7d96,0x7e64,0x7f32,
};

static void sine_run(void const *arg)
{
	static uint16_t curTimeMs = 0;

	osMutexWait(M_sine_state, osWaitForever);
	{
		sine_state_t const *state = arg;

		if (curTimeMs >= state->periodMs) {
			curTimeMs = 0;
		}

		uint16_t idx = (uint32_t)curTimeMs * SINE_LOOKUP_SZ / state->periodMs;
		GPIO_Write(WAVEFORM_PORT, (uint32_t)state->amplitude * sine_lookup[idx] / 0xFFFF);
	}
	osMutexRelease(M_sine_state);

	++curTimeMs;
}
