/*	SC	A Spreadsheet Calculator
 *		Routines for piping data to and from an external macro program
 *
 *		Chuck Martin <cmartin@bigfoot.com>
 *		Original Version Created:  June, 2000
 *
 *		$Revision: 7.13 $
 */

#include <curses.h>
#include <unistd.h>
#include "sc.h"

#ifdef BSD42
#include <strings.h>
#include <sys/time.h>
#ifndef strchr
#define strchr index
#endif
#else
#include <time.h>
#ifndef SYSIII
#include <string.h>
#endif
#endif


void
getnum(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    struct ent	*p;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    p = *pp;
	    if (p)
		if (p->cellerror)
		    sprintf(line, "%s", (*pp)->cellerror == CELLERROR ?
			    "ERROR" : "INVALID");
		else if (p->flags & is_valid)
		    sprintf(line, "%.15g", p->v);
	    if (c < cn)
		strcat(line, "\t");
	    else
		strcat(line, "\n");
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
fgetnum(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    struct ent	*p;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    p = *pp;
	    if (p) {
		if (p->cellerror)
		    sprintf(line, "%s", p->cellerror == CELLERROR ?
			    "ERROR" : "INVALID");
		else if (p->flags & is_valid) {
		    if (p->format) {
			if (*(p->format) == ctl('d')) {
			    time_t i = (time_t) (p->v);
			    strftime(line, sizeof(line), (p->format)+1,
				localtime(&i));
			} else
			    format(p->format, precision[c], p->v, line,
				    sizeof(line));
		    } else
			engformat(realfmt[c], fwidth[c], precision[c],
				p->v, line, sizeof(line));
		}
	    }
	    if (c < cn)
		strcat(line, "\t");
	    else
		strcat(line, "\n");
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
getstring(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    if (*pp && (*pp)->label)
		sprintf(line, "%s", (*pp)->label);
	    if (c < cn)
		strcat(line, "\t");
	    else
		strcat(line, "\n");
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
getexp(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    struct ent	*p;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    p = *pp;
	    if (p && p->expr) {
		linelim = 0;
		decompile(p->expr, 0);	/* set line to expr */
		line[linelim] = '\0';
		if (*line == '?')
		    *line = '\0';
	    }
	    if (c < cn)
		strcat(line, "\t");
	    else
		strcat(line, "\n");
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
getformat(int col, int fd)
{
    sprintf(line, "%d %d %d\n", fwidth[col], precision[col], realfmt[col]);
    write(fd, line, strlen(line));
    linelim = -1;
}

void
getfmt(int r0, int c0, int rn, int cn, int fd)
{
    struct ent	**pp;
    int		r, c;

    for (r = r0; r <= rn; r++) {
	for (c = c0, pp = ATBL(tbl, r, c); c <= cn; pp++, c++) {
	    *line = '\0';
	    if (*pp && (*pp)->format)
		sprintf(line, "%s", (*pp)->format);
	    if (c < cn)
		strcat(line, "\t");
	    else
		strcat(line, "\n");
	    write(fd, line, strlen(line));
	    if (brokenpipe) {
		linelim = -1;
		return;
	    }
	}
    }
    linelim = -1;
}

void
getframe(int fd)
{
    struct frange *fr;

    *line = '\0';
    if ((fr = find_frange(currow, curcol))) {
	sprintf(line, "%s", r_name(fr->or_left->row, fr->or_left->col,
		fr->or_right->row, fr->or_right->col));
	strcat(line, " ");
	sprintf(line + strlen(line), "%s", r_name(fr->ir_left->row,
		fr->ir_left->col, fr->ir_right->row, fr->ir_right->col));
    }
    strcat(line, "\n");
    write(fd, line, strlen(line));
    linelim = -1;
}

void
doeval(struct enode *e, char *fmt, int row, int col, int fd)
{
    double v;

    gmyrow = row;
    gmycol = col;

    v = eval(e);
    if (fmt) {
	if (*fmt == ctl('d')) {
	    time_t tv = v;
	    strftime(line, FBUFLEN, fmt + 1, localtime(&tv));
	} else
	    format(fmt, precision[col], v, line, FBUFLEN);
    } else
	sprintf(line, "%.15g", v);
    strcat(line, "\n");
    write(fd, line, strlen(line));
    linelim = -1;

    efree(e);
    if (fmt) scxfree(fmt);
}

void
doseval(struct enode *e, int row, int col, int fd)
{
    char *s;

    gmyrow = row;
    gmycol = col;

    s = seval(e);
    write(fd, s, strlen(s));
    write(fd, "\n", 1);
    linelim = -1;

    efree(e);
    scxfree(s);
}


void
doquery(char *s, char *data, int fd)
{
    query(s, data);
    if (linelim >= 0)
	write(fd, line, strlen(line));

    line[0] = '\0';
    linelim = -1;
    error("");
    update(0);

    if (s) scxfree(s);
}

void
dostat(int fd)
{
    *line = '\0';
    if (modflg)			sprintf(line, "m");
    if (isatty(STDIN_FILENO))	strcat(line, "i");
    if (isatty(STDOUT_FILENO))	strcat(line, "o");
    strcat(line, "\n");
    write(fd, line, strlen(line));
    linelim = -1;
}
