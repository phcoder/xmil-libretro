
typedef struct {
const UINT8	*src;
	UINT8	*dst;
	int		width;
	int		xbytes;
	int		y;
	int		xalign;
	int		yalign;
	UINT8	dirty[SURFACE_HEIGHT];
} _SDRAW, *SDRAW;

typedef void (SCRNCALL * SDRAWFN)(SDRAW sdraw, int maxy);


#ifdef __cplusplus
extern "C" {
#endif

const SDRAWFN *sdraw_getproctbl(const SCRNSURF *surf);

#ifdef __cplusplus
}
#endif
