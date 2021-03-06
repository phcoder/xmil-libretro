
enum {
	ADVKEY_A		= 0x0001,
	ADVKEY_B		= 0x0002,
	ADVKEY_SELECT	= 0x0004,
	ADVKEY_START	= 0x0008,
	ADVKEY_RIGHT	= 0x0010,
	ADVKEY_LEFT		= 0x0020,
	ADVKEY_UP		= 0x0040,
	ADVKEY_DOWN		= 0x0080,
	ADVKEY_R		= 0x0100,
	ADVKEY_L		= 0x0200
};

#define	REG_KEYINPUT	((UINT16 *)(__adv_reg + 0x130))


// ----

#ifdef __cplusplus
extern "C" {
#endif

void advkey_initialize(void);
void advkey_keydown(UINT16 key);
void advkey_keyup(UINT16 key);

#ifdef __cplusplus
}
#endif

