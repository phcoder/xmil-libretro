/**
 * @file	filesel.c
 * @brief	Implementation of the selection of files
 */

#include "compiler.h"
#include "filesel.h"
#include "../menubase/menudlg.h"
#include "../menubase/menures.h"
#include "../menubase/menuicon.h"
#include "strres.h"
#include "dosio.h"
#include "soundmng.h"
#include "pccore.h"
#include "fdd/diskdrv.h"
#include "fdd/fddfile.h"

enum {
	DID_FOLDER	= DID_USER,
	DID_PARENT,
	DID_FLIST,
	DID_FILE,
	DID_FILTER
};

static const OEMCHAR str_dirname[] = OEMTEXT("Look in");
static const OEMCHAR str_filename[] = OEMTEXT("File name");
static const OEMCHAR str_filetype[] = OEMTEXT("Files of type");
static const OEMCHAR str_open[] = OEMTEXT("Open");

#if defined(SIZE_QVGA)
enum {
	DLGFS_WIDTH		= 294,
	DLGFS_HEIGHT	= 187
};
static const MENUPRM res_fs[] = {
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_dirname,							  6,   9,  68,  11},
			{DLGTYPE_EDIT,		DID_FOLDER,		0,
				NULL,									 74,   6, 192,  16},
			{DLGTYPE_BUTTON,	DID_PARENT,		MENU_TABSTOP,
				NULL,									272,   6,  16,  16},
			{DLGTYPE_LIST,		DID_FLIST,		MENU_TABSTOP,
				NULL,									  5,  28, 284, 115},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_filename,							  6, 150,  68,  11},
			{DLGTYPE_EDIT,		DID_FILE,		0,
				NULL,									 74, 147, 159,  16},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_filetype,							  6, 169,  68,  11},
			{DLGTYPE_EDIT,		DID_FILTER,		0,
				NULL,									 74, 166, 159,  16},
			{DLGTYPE_BUTTON,	DID_OK,			MENU_TABSTOP,
				str_open,								237, 147,  52,  15},
			{DLGTYPE_BUTTON,	DID_CANCEL,		MENU_TABSTOP,
				mstr_cancel,							237, 166,  52,  15}};
#else
enum {
	DLGFS_WIDTH		= 499,
	DLGFS_HEIGHT	= 227
};
static const MENUPRM res_fs[] = {
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_dirname,							 12,  10, 102,  13},
			{DLGTYPE_EDIT,		DID_FOLDER,		0,
				NULL,									114,   7, 219,  18},
			{DLGTYPE_BUTTON,	DID_PARENT,		MENU_TABSTOP,
				NULL,									348,   7,  18,  18},
			{DLGTYPE_LIST,		DID_FLIST,		MENU_TABSTOP,
				NULL,									  7,  30, 481, 128},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_filename,							 12, 168, 104,  13},
			{DLGTYPE_EDIT,		DID_FILE,		0,
				NULL,									116, 165, 268,  18},
			{DLGTYPE_LTEXT,		DID_STATIC,		0,
				str_filetype,							 12, 192, 104,  13},
			{DLGTYPE_EDIT,		DID_FILTER,		0,
				NULL,									116, 189, 268,  18},
			{DLGTYPE_BUTTON,	DID_OK,			MENU_TABSTOP,
				str_open,								397, 165,  88,  23},
			{DLGTYPE_BUTTON,	DID_CANCEL,		MENU_TABSTOP,
				mstr_cancel,							397, 192,  88,  23}};
#endif

struct _flist;
typedef struct _flist	 _FLIST;
typedef struct _flist	 *FLIST;

struct _flist {
	FLIST	next;
	UINT	isdir;
	OEMCHAR	name[MAX_PATH];
};

typedef struct {
const OEMCHAR	*title;
const OEMCHAR	*filter;
const OEMCHAR	*ext;
int drv;
} FSELPRM;

typedef struct {
	BOOL		result;
	LISTARRAY	flist;
	FLIST		fbase;
const OEMCHAR	*filter;
const OEMCHAR	*ext;
	OEMCHAR		path[MAX_PATH];
int drv;
} FILESEL;

static	FILESEL		filesel;


// ----

static FLIST getflist(int pos) {

	FLIST	ret;

	ret = NULL;
	if (pos >= 0) {
		ret = filesel.fbase;
		while((pos > 0) && (ret)) {
			pos--;
			ret = ret->next;
		}
	}
	return(ret);
}

static BRESULT fappend(LISTARRAY flist, FLINFO *fli) {

	FLIST	fl;
	FLIST	*st;
	FLIST	cur;

	fl = (FLIST)listarray_append(flist, NULL);
	if (fl == NULL) {
		return(FAILURE);
	}
	fl->isdir = (fli->attr & 0x10)?1:0;
	file_cpyname(fl->name, fli->path, NELEMENTS(fl->name));
	st = &filesel.fbase;
	while(1) {
		cur = *st;
		if (cur == NULL) {
			break;
		}
		if (fl->isdir > cur->isdir) {
			break;
		}
		if ((fl->isdir == cur->isdir) &&
			(file_cmpname(fl->name, cur->name) < 0)) {
			break;
		}
		st = &cur->next;
	}
	fl->next = *st;
	*st = fl;
	return(SUCCESS);
}

static BOOL checkext(const OEMCHAR *path, const OEMCHAR *ext) {

const OEMCHAR	*p;

	if (ext == NULL) {
		return(TRUE);
	}
	p = file_getext(path);
	while(*ext) {
		if (!file_cmpname(p, ext)) {
			return(TRUE);
		}
		ext += OEMSTRLEN(ext) + 1;
	}
	return(FALSE);
}

static void dlgsetlist(void) {

	LISTARRAY	flist;
	FLISTH		flh;
	FLINFO		fli;
	BOOL		append;
	FLIST		fl;
	ITEMEXPRM	prm;

	menudlg_itemreset(DID_FLIST);
	menudlg_settext(DID_FOLDER, file_getname(filesel.path));
	listarray_destroy(filesel.flist);
	flist = listarray_new(sizeof(_FLIST), 64);
	filesel.flist = flist;
	filesel.fbase = NULL;
	flh = file_list1st(filesel.path, &fli);
	if (flh != FLISTH_INVALID) {
		do {
			append = FALSE;
			if (fli.attr & 0x10) {
				append = TRUE;
			}
			else if (!(fli.attr & 0x08)) {
				append = checkext(fli.path, filesel.ext);
			}
			if (append) {
				if (fappend(flist, &fli) != SUCCESS) {
					break;
				}
			}
		} while(file_listnext(flh, &fli) == SUCCESS);
		file_listclose(flh);
	}
	prm.pos = 0;
	fl = filesel.fbase;
	while(fl) {
		menudlg_itemappend(DID_FLIST, NULL);
		prm.icon = (fl->isdir)?MICON_FOLDER:MICON_FILE;
		prm.str = fl->name;
		menudlg_itemsetex(DID_FLIST, &prm);
		fl = fl->next;
		prm.pos++;
	}
}

static void dlginit(void) {

	menudlg_appends(res_fs, NELEMENTS(res_fs));
	menudlg_seticon(DID_PARENT, MICON_FOLDERPARENT);
	menudlg_settext(DID_FILE, file_getname(filesel.path));
	menudlg_settext(DID_FILTER, filesel.filter);
	file_cutname(filesel.path);
	file_cutseparator(filesel.path);
	dlgsetlist();
}

static BOOL dlgupdate(void) {

	FLIST	fl;

	fl = getflist(menudlg_getval(DID_FLIST));
	if (fl == NULL) {
		return(FALSE);
	}
	file_setseparator(filesel.path, NELEMENTS(filesel.path));
	file_catname(filesel.path, fl->name, NELEMENTS(filesel.path));
	if (fl->isdir) {
		dlgsetlist();
		menudlg_settext(DID_FILE, NULL);
		return(FALSE);
	}
	else {
		filesel.result = TRUE;
		return(TRUE);
	}
}

static void dlgflist(void) {

	FLIST	fl;

	fl = getflist(menudlg_getval(DID_FLIST));
	if ((fl != NULL) && (!fl->isdir)) {
		menudlg_settext(DID_FILE, fl->name);
	}
}

static int dlgcmd(int msg, MENUID id, long param) {

	switch(msg) {
		case DLGMSG_CREATE:
			dlginit();
			break;

		case DLGMSG_COMMAND:
			switch(id) {
				case DID_OK:
					if (dlgupdate()) {
						diskdrv_setfdd(filesel.drv, filesel.path, 0);
						menubase_close();
					}
					break;

				case DID_CANCEL:
					menubase_close();
					break;

				case DID_PARENT:
					file_cutname(filesel.path);
					file_cutseparator(filesel.path);
					dlgsetlist();
					menudlg_settext(DID_FILE, NULL);
					break;

				case DID_FLIST:
					if (param) {
						return(dlgcmd(DLGMSG_COMMAND, DID_OK, 0));
					}
					else {
						dlgflist();
					}
					break;
			}
			break;

		case DLGMSG_CLOSE:
			menubase_close();
			break;

		case DLGMSG_DESTROY:
			listarray_destroy(filesel.flist);
			filesel.flist = NULL;
			break;
	}
	(void)param;
	return(0);
}

static BOOL selectfile(const FSELPRM *prm, OEMCHAR *path, int size, 
														const OEMCHAR *def,int drv) {

const OEMCHAR	*title;

	soundmng_stop();
	ZeroMemory(&filesel, sizeof(filesel));
	if ((def) && (def[0])) {
		file_cpyname(filesel.path, def, NELEMENTS(filesel.path));
	}
	else {
		file_cpyname(filesel.path, file_getcd(str_null),
													NELEMENTS(filesel.path));
		file_cutname(filesel.path);
	}
	title = NULL;
	if (prm) {
		title = prm->title;
		filesel.filter = prm->filter;
		filesel.ext = prm->ext;
		filesel.drv = drv;
	}
	menudlg_create(DLGFS_WIDTH, DLGFS_HEIGHT, title, dlgcmd);
	//menubase_modalproc();
	soundmng_play();
	if (filesel.result) {
		file_cpyname(path, filesel.path, size);
		return(TRUE);
	}
	else {
		return(FALSE);
	}
}


// ----

static const OEMCHAR diskfilter[] = OEMTEXT("All supported files");
static const OEMCHAR fddtitle[] = OEMTEXT("Select floppy image");
static const OEMCHAR fddext[] = OEMTEXT("d88\0") OEMTEXT("88d\0") OEMTEXT("dx1\0") OEMTEXT("2d\0") OEMTEXT("xdf\0") OEMTEXT("hdm\0") OEMTEXT("dup\0") OEMTEXT("2hd\0") OEMTEXT("tfd\0");
static const FSELPRM fddprm = {fddtitle, diskfilter, fddext};


void filesel_fdd(REG8 drv) {

	OEMCHAR	path[MAX_PATH];

	if (drv < 4) {
		if (selectfile(&fddprm, path, NELEMENTS(path), fddfile_diskname(drv),drv)) {
			diskdrv_setfdd(drv, path, 0);
		}
	}
}

