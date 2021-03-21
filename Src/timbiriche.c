// ----------------------------------------------------------------
// Timbiriche
//     v. 1.2.0
//     Copyright ? 2004-2005 Gonzalo Mena-Mendoza
//     gonzalo@mena.com.mx
//     http://mena.com.mx
// ----------------------------------------------------------------
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
// 
// You should have received a copy of the GNU General Public
// License along with this program; if not, write to the Free
// Software Foundation, Inc., 59 Temple Place, Suite 330,
// Boston, MA  02111-1307  USA
// ----------------------------------------------------------------
#include <PalmOS.h>
#include <PalmOSGlue.h>

#include "timbiriche.h"
#include "timbiriche_Rsc.h"

#define AppCreator 'Tche'
#define ColorROMVersion  sysMakeROMVersion(3,5,0,sysROMStageDevelopment,0)
#define MinROMVersion    sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0)
#define VersionRes 1

#define MaxPlayers 4
#define MaxHoriz 10
#define MaxVert 9
#define MinPlayers 2
#define MinHoriz 3
#define MinVert 3
#define FirstHoriz 6
#define FirstVert 6
#define TypeHuman 0
#define TypeEasy 1
#define TypeMedium 2
#define TypeHoriz 0
#define TypeVert 1
#define CheatChr 0x54
#define Invalid -1
#define RefreshPerSec 10

#define EsLocale 0
#define EnLocale 1000
#define PtLocale 2000

#define MainForm 1000
#define LocaleForm 1001
#define PrefsForm 1002
#define AboutForm 1003

#define MainMenuBar 1000
#define NewMenuOpt 1000
#define AboutMenuOpt 1004
#define PrefsMenuOpt 1005
#define HowtoMenuOpt 1006

#define EndBaseAlert 1010
#define IncompatAlert 1100
#define DebugAlert 1101

#define PrefsPlayerBaseButton 1100
#define PrefsTypeBaseButton 1200
#define PrefsTypeBaseList 1201
#define PrefsTypeDelta 10
#define PrefsHorizButton 1300
#define PrefsHorizList 1301
#define PrefsVertButton 1310
#define PrefsVertList 1311
#define PrefsLocaleButton 1400
#define PrefsLocaleList 1401
#define PrefsOKButton 1900

#define AboutVersionLabel 1001
#define AboutCreditsButton 1010
#define AboutLicenseButton 1011
#define AboutOKButton 1100
#define AboutCreditsStr 1001
#define AboutLicenseStr 1900

#define PlayerBaseButton 1800
#define NewGameButton 1900
#define DotBmp 1000
#define NumberBaseBmp 1100
#define PenBaseBmp 1200
#define ScoreFormatStr 1000
#define NewGameStr 1003
#define HowtoStr 1004

#define ScreenW 160
#define ScreenH 160
#define BoardX 0
#define BoardY 16
#define CornerSize 12
#define DotSize 6
#define NumberSize 13

typedef struct {
    int numPlayers;
    int numHoriz;
    int numVert;
    int locale;
    int playerType[MaxPlayers];
    int horiz[MaxHoriz][MaxVert + 1];
    int vert[MaxHoriz + 1][MaxVert];
    int squares[MaxHoriz][MaxVert];
    int currPlayer;
} Prefs;
Prefs prefs;

typedef struct {
    int x;
    int y;
    int lineType;
    int weight;
} Line;
Line candidates[2 * (MaxHoriz + 1) * (MaxVert + 1)];

RGBColorType squareBack[MaxPlayers] = {
    {0x00, 0xff, 0x00, 0x00},
    {0x00, 0x00, 0xff, 0x00},
    {0x00, 0x00, 0x00, 0xff},
    {0x00, 0xff, 0xff, 0x00}
};
RGBColorType squareHigh[MaxPlayers] = {
    {0x00, 0xff, 0x99, 0x99},
    {0x00, 0x99, 0xff, 0x99},
    {0x00, 0x99, 0x99, 0xff},
    {0x00, 0xff, 0xff, 0x99}
};
RGBColorType squareLow[MaxPlayers] = {
    {0x00, 0x66, 0x00, 0x00},
    {0x00, 0x00, 0x66, 0x00},
    {0x00, 0x00, 0x00, 0x66},
    {0x00, 0x66, 0x66, 0x00}
};

char *versionP = "1.x.ya";
char *scoreLabelsP[MaxPlayers] =
     {"N: 999", "N: 999", "N: 999", "N: 999"};
char *scoreFmtP = "N: 999";
char *newGameStrP = "Nueva PartidaNueva Partida";

int scores[MaxPlayers];
int sideSize;
int homeX;
int homeY;
int startX;
int startY;
int oneSecond;
Boolean inMenu;

static ControlPtr getCtlPtr(FormPtr frmP, int b) {
    return FrmGetObjectPtr(frmP, 
        FrmGetObjectIndex(frmP, b));
}

static ListPtr getLstPtr(FormPtr frmP, int b) {
    return FrmGetObjectPtr(frmP, 
        FrmGetObjectIndex(frmP, b));
}

static void copyStrFromRes(Char *dst, UInt32 type, Int16 id) {
    MemHandle nH = DmGetResource(type, id);
    StrCopy(dst, (Char *) MemHandleLock(nH));
    MemHandleUnlock(nH);
    DmReleaseResource(nH);
}

static int ctlGrpSel(FormPtr frmP, int baseButton, int count) {
    int i;
    for (i = 0; i < count; ++i)
        if (CtlGetValue(getCtlPtr(frmP,
            baseButton + i)))
            return i;
    return -1;
}

static void lstPopSet(FormPtr frmP, int list, int button, int index) {
    ListPtr listP = getLstPtr(frmP, list);
    ControlPtr buttonP = getCtlPtr(frmP, button);
    LstSetSelection(listP, index);
    LstMakeItemVisible(listP, index);
    CtlSetLabel(buttonP,
        LstGetSelectionText(listP, index));
}

Boolean isCompatible() { 	
    UInt32 romVer;
    UInt32 compVer = MinROMVersion;
    Err err = FtrGet(sysFtrCreator,
        sysFtrNumROMVersion, &romVer);
        
    return (romVer >= MinROMVersion);
}

static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	/* See if we're on in minimum required version of the ROM or later. */
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
	{
		return sysErrRomIncompatible;
	}

	return errNone;
}


Boolean supportsColor() {
    UInt32 romVer;
    UInt32 compVer = ColorROMVersion;
    Err err = FtrGet(sysFtrCreator,
        sysFtrNumROMVersion, &romVer);
    RomVersionCompatible(MinROMVersion, 0);    
    return !(romVer < compVer);
}

static void debugShow(int a, int b, int c) {
    char sa[10], sb[10], sc[10];
    StrIToA(sa, a);
    StrIToA(sb, b);
    StrIToA(sc, c);
    FrmCustomAlert(DebugAlert, sa, sb, sc);
}

static int playerToControl(int player) {
   return PlayerBaseButton
       + (MaxPlayers - prefs.numPlayers)
       + player;
}

static void writeScore(FormPtr frmP, int player) {
    int b = playerToControl(player);
    StrPrintF(scoreLabelsP[player],
        scoreFmtP, player+1, scores[player]);
    CtlSetLabel(getCtlPtr(frmP, b), scoreLabelsP[player]);
}

static void nextPlayer(FormPtr frmP) {
	int i;
    ++prefs.currPlayer;
    prefs.currPlayer %= prefs.numPlayers;
    for (i = 0; i < prefs.numPlayers; ++i) {
        int b = playerToControl(i);
        CtlSetValue(getCtlPtr(frmP, b),
            ((i == prefs.currPlayer) ? 1 : 0));
    }
}

static void drawBmp(int bmp, int px, int py) {
    MemHandle bmpH =  DmGetResource(
        'Tbmp', bmp);
    BitmapPtr bmpP = MemHandleLock(bmpH);
    WinDrawBitmap(bmpP, px, py);
    MemPtrUnlock(bmpP);
    DmReleaseResource(bmpH);    
}

static void drawCorner(int x, int y) {
    drawBmp(DotBmp,
        homeX + x*sideSize + ((CornerSize - DotSize) / 2),
        homeY + y*sideSize + ((CornerSize - DotSize) / 2)
    );
}

static void drawHoriz(int x, int y) {
    int cx = homeX + x*sideSize + CornerSize/2 + DotSize/2;
    int cy = homeY + y*sideSize + CornerSize/2;
    int len = sideSize - DotSize;
    WinDrawLine(cx, cy, cx + len, cy);
    WinDrawLine(cx, cy - 1, cx + len, cy - 1);
}

static void drawVert(int x, int y) {
    int cx = homeX + x*sideSize + CornerSize/2;
    int cy = homeY + y*sideSize + CornerSize/2 + DotSize/2;
    int len = sideSize - DotSize;
    WinDrawLine(cx, cy, cx, cy + len);
    WinDrawLine(cx - 1, cy, cx - 1, cy + len);
}

static void fillSquare(int x, int y, int i) {
	RectangleType r;
    int cx = homeX + x*sideSize;
    int cy = homeY + y*sideSize;
    int rx = cx + CornerSize/2 + 1;
    int ry = cy + CornerSize/2 + 1;
    int l = sideSize - 2;
    int d;
    if(supportsColor())
    	WinPushDrawState();
    // rectangle
    RctSetRectangle(&r, rx, ry, l, l);
    if(supportsColor())
    	WinSetBackColor(WinRGBToIndex(&squareBack[i]));
    WinEraseRectangle(&r, 0);
    WinDrawRectangleFrame(simpleFrame, &r);
    // highlights
    if(supportsColor())
    	WinSetForeColor(WinRGBToIndex(&squareHigh[i]));
    WinDrawLine(rx, ry, rx + l, ry);
    WinDrawLine(rx, ry, rx, ry + l);
    if(supportsColor())
    	WinSetForeColor(WinRGBToIndex(&squareLow[i]));
    WinDrawLine(rx, ry + l - 1, rx + l - 1, ry + l - 1);
    WinDrawLine(rx + l - 1, ry, rx + l - 1, ry + l - 1);    
    // number
    d = CornerSize/2 + (sideSize - NumberSize)/2;
    drawBmp(NumberBaseBmp + i,
        cx + d, cy + d);
    // corners
    drawCorner(x, y);
    drawCorner(x + 1, y);
    drawCorner(x, y + 1);
    drawCorner(x + 1, y + 1);
    if(supportsColor())
    	WinPopDrawState();
}

static int countLines(int x, int y) {
    return prefs.horiz[x][y]
        + prefs.vert[x][y]
        + prefs.horiz[x][y + 1]
        + prefs.vert[x + 1][y];
}

static int checkSquare(int x, int y) {
    if (4 == countLines(x, y)) {
        prefs.squares[x][y] =
            prefs.currPlayer + 1;
        fillSquare(x, y, prefs.currPlayer);
        return 1;
    }
    return 0;
}

static void end(FormPtr frmP) {
    int i;
    char s1[4], s2[4], s3[4];
    
    // find out who won
    int winners[MaxPlayers];
    int numWin = 1;    
    winners[0] = 0;
    for (i = 1; i < prefs.numPlayers; ++i) {
        if (scores[i] > scores[winners[0]]) {
            winners[0] = i;
            numWin = 1;
        }
        else if (scores[i] == scores[winners[0]]) {
            winners[numWin] = i;
            ++numWin;
        }
    }
    // show winner info
    StrIToA(s1, winners[0]+1);
    if (numWin>1) StrIToA(s2, winners[1]+1);
    if (numWin>2) StrIToA(s3, winners[2]+1);
    FrmCustomAlert(EndBaseAlert + numWin + prefs.locale,
        s1, s2, s3);
    // show "new game" button
    CtlShowControl(getCtlPtr(frmP,
        NewGameButton));
    // indicate end of game
    prefs.currPlayer = Invalid;    
}

static Boolean hasFinished() {
    int i, sum = 0;
    for (i = 0; i < prefs.numPlayers; ++i)
        sum += scores[i];
    if (sum < (prefs.numHoriz*prefs.numVert))
        return false;
    else
        return true;
}

static void play(FormPtr frmP, int x, int y, int lineType) {
    int points = 0;

    if (TypeHoriz == lineType) {
        // draw
        prefs.horiz[x][y] = 1;
        drawHoriz(x, y);
        // check
        if (y > 0)
            points += checkSquare(x, y - 1);
        if (y < prefs.numVert)
            points += checkSquare(x, y);
    }
    else {
        // draw
        prefs.vert[x][y] = 1;
        drawVert(x, y);
        // check
        if (x > 0)
            points += checkSquare(x - 1, y);
        if (x < prefs.numHoriz)
            points += checkSquare(x, y);
    }
    
    if (points) {
        // continue
        scores[prefs.currPlayer] += points;
        writeScore(frmP, prefs.currPlayer);
        // end?
        if (hasFinished())
            end(frmP);
    } else {
        // next
        nextPlayer(frmP);
    }
}

static Boolean checkLine(FormPtr frmP, int px0, int py0, int px1, int py1) {
    int x0 = (px0 - homeX) / sideSize;
    int y0 = (py0 - homeY) / sideSize;
    int x1 = (px1 - homeX) / sideSize;
    int y1 = (py1 - homeY) / sideSize;
    
    int rx0 = (px0 - homeX) % sideSize;
    int ry0 = (py0 - homeY) % sideSize;
    int rx1 = (px1 - homeX) % sideSize;
    int ry1 = (py1 - homeY) % sideSize;
        
    // outside of board
    if ( (px0 < homeX) || (px1 < homeX)
        || (py0 < homeY) || (py1 < homeY)
        || (x0 < 0) || (x0 > prefs.numHoriz)
        || (y0 < 0) || (y0 > prefs.numVert)
        || (x1 < 0) || (x1 > prefs.numHoriz)
        || (y1 < 0) || (y1 > prefs.numVert)
    )
        return false;
    
    // outside of corner
    if ( (rx0 > CornerSize) || (ry0 > CornerSize)
        || (rx1 > CornerSize) || (ry1 > CornerSize)
    )
        return false;

    // horizontal
    if ((y0 == y1) && ((x0 == x1+1) || (x0 == x1-1))) {
        int x = (x0 < x1) ? x0 : x1;
        int y = y0;
        // existing
        if (prefs.horiz[x][y])
            return false;
        play(frmP, x, y, TypeHoriz);
        return true;
    }
    // vertical
    else
    if ((x0 == x1) && ((y0 == y1+1) || (y0 == y1-1))) {
        int x = x0;
        int y = (y0 < y1) ? y0 : y1;
        // existing
        if (prefs.vert[x][y])
            return false;
        play(frmP, x, y, TypeVert);
        return true;
    }
    // none
    else return false;
}

static int weightMed(int x, int y, int lineType) {
	int i, w = 0;
    int c[2] = {0, 0};
    if (TypeHoriz == lineType) {
        if (y > 0)
            c[0] = countLines(x, y-1);
        if (y < prefs.numVert)
            c[1] = countLines(x, y);        
    }
    else {
        if (x > 0)
            c[0] = countLines(x - 1, y);
        if (x < prefs.numHoriz)
            c[1] = countLines(x, y);        
    }
 
    for (i = 0; i < 2; ++i)
        w += (3 == c[i]) ? 2
            : ( (2 == c[i]) ? -1
            : 0 );
    //if (w != 0)
    //    debugShow(lineType, x*10 + y, w);
    return w;
}

static int sortCandidates(int total) {
    // bubble sort
    int max, w, i, j, aX, aY, aT, aW;
    for (i = total; i > 0; --i)
        for (j = 1; j < i; ++j)
            if (candidates[j-1].weight < candidates[j].weight) {
                // swap
                aX = candidates[j-1].x;
                aY = candidates[j-1].y;
                aT = candidates[j-1].lineType;
                aW = candidates[j-1].weight;
                candidates[j-1].x = candidates[j].x;
                candidates[j-1].y = candidates[j].y;
                candidates[j-1].lineType = candidates[j].lineType;
                candidates[j-1].weight = candidates[j].weight;
                candidates[j].x = aX;
                candidates[j].y = aY;
                candidates[j].lineType = aT;
                candidates[j].weight = aW;
            }
    // max to consider
    max = 0;
    w = candidates[0].weight;
    for (max = 1; max < total; ++max)
        if (w != candidates[max].weight)
            break;
    return max;
}

static void autoPlay(FormPtr frmP) {
    // identify candidate moves
    int i, x, y, tc = 0; // count
    // horiz
    for (x = 0; x < prefs.numHoriz; ++x)
        for (y = 0; y < (prefs.numVert + 1); ++y)
            if (0 == prefs.horiz[x][y]) {
                candidates[tc].x = x;
                candidates[tc].y = y;
                candidates[tc].lineType = TypeHoriz;
                if (TypeMedium == prefs.playerType[prefs.currPlayer])
                    candidates[tc].weight =
                        weightMed(x, y, TypeHoriz);
                ++tc; 
            }
    // vert
    for (x = 0; x < (prefs.numHoriz + 1); ++x)
        for (y = 0; y < prefs.numVert; ++y)
            if (0 == prefs.vert[x][y]) {
                candidates[tc].x = x;
                candidates[tc].y = y;
                candidates[tc].lineType = TypeVert;
                if (TypeMedium == prefs.playerType[prefs.currPlayer])
                    candidates[tc].weight =
                        weightMed(x, y, TypeVert);
                ++tc; 
            }
    // choose randomly
    if (TypeMedium == prefs.playerType[prefs.currPlayer])
        tc = sortCandidates(tc);
    i = SysRandom(0) % tc;
    play(frmP,
        candidates[i].x, candidates[i].y,
        candidates[i].lineType);
}

static void init() {
	int x, y;
    // squares
    for (x = 0; x < MaxHoriz; ++x)
        for (y = 0; y < MaxVert; ++y)
            prefs.squares[x][y] = 0;
    // horiz
    for (x = 0; x < MaxHoriz; ++x)
        for (y = 0; y < (MaxVert + 1); ++y)
            prefs.horiz[x][y] = 0;
    // vert
    for (x = 0; x < (MaxHoriz + 1); ++x)
        for (y = 0; y < MaxVert; ++y)
            prefs.vert[x][y] = 0;

    prefs.currPlayer = 0;
}

static void refresh(FormPtr frmP) {
    int i, x, y, boardW, boardH, sideX, sideY;
	RectangleType r;
	int b;
    ControlPtr bP;

   	// clear scores, to be recalculated
    for (i = 0; i < MaxPlayers; ++i)
        scores[i] = 0;
    
    // hide button
    CtlHideControl(getCtlPtr(frmP,
        NewGameButton));        

    // square size
    boardW = ScreenW - BoardX - CornerSize;
    boardH = ScreenH - BoardY - CornerSize;
    sideX =  boardW / prefs.numHoriz;
    sideY = boardH / prefs.numVert;
    sideSize = (sideX <sideY) ? sideX : sideY;
    homeX = BoardX +
        (boardW - sideSize*prefs.numHoriz)/2;
    homeY = BoardY +
        (boardH - sideSize*prefs.numVert)/2;

    // clear screen
    RctSetRectangle(&r, BoardX, BoardY,
        ScreenW - BoardX, ScreenH - BoardY);
    WinEraseRectangle(&r, 0);

    // draw grid
    for (x = 0; x < (prefs.numHoriz + 1); ++x)
        for (y = 0; y < (prefs.numVert + 1); ++y) {
            // horiz
            if ((x < prefs.numHoriz)
                && prefs.horiz[x][y])
                drawHoriz(x, y);
            // vert
            if ((y < prefs.numVert)
                && prefs.vert[x][y])
                drawVert(x, y);
            // square
            if ((x < prefs.numHoriz)
                && (y < prefs.numVert)
                && prefs.squares[x][y])
            {
                int p = prefs.squares[x][y] - 1;
                ++scores[p];
                fillSquare(x, y, p);
            }
            // corner
            else {
                drawCorner(x, y);
            }
        }

    // show scores
    for (i = 0; i < MaxPlayers; ++i)
        CtlHideControl(getCtlPtr(frmP,
            PlayerBaseButton + i));
    for (i = 0 ; i < prefs.numPlayers; ++i) {
        b = playerToControl(i);
        bP = getCtlPtr(frmP, b);
        CtlShowControl(bP);
        CtlSetValue(getCtlPtr(frmP, b),
            ((i == prefs.currPlayer) ? 1 : 0));
        writeScore(frmP, i);
    }

    // show "new game" button
    if (hasFinished())
        CtlShowControl(getCtlPtr(frmP,
            NewGameButton));        
}

static void changeLocale(FormPtr frmP) {
    // menu
    FrmSetMenu(frmP,
        MainMenuBar + prefs.locale);   
    // new game button label
    copyStrFromRes(newGameStrP,
        'tSTR', NewGameStr + prefs.locale);
    CtlSetLabel(getCtlPtr(frmP,
        NewGameButton), newGameStrP);
}

static void showPrefs(FormPtr frmP) {
    // current values
    Boolean mustInit = false;
    int oldNumPlayers = prefs.numPlayers;
    int oldNumHoriz = prefs.numHoriz;
    int oldNumVert = prefs.numVert;
    int oldPlayerType[MaxPlayers];
    int i;
    FormPtr alertP;
    
    for (i = 0; i < MaxPlayers; ++i)
        oldPlayerType[i] = prefs.playerType[i];
    
    // init form to current values
    alertP = FrmInitForm(
        PrefsForm + prefs.locale);
    CtlSetValue(getCtlPtr(alertP,
        PrefsPlayerBaseButton + prefs.numPlayers - MinPlayers), 1);
    for (i = 0; i < MaxPlayers; ++i)
        lstPopSet(alertP,
            PrefsTypeBaseList + i * PrefsTypeDelta,
            PrefsTypeBaseButton + i * PrefsTypeDelta,
            prefs.playerType[i]);
    lstPopSet(alertP, PrefsHorizList, PrefsHorizButton,
        prefs.numHoriz - MinHoriz);
    lstPopSet(alertP, PrefsVertList, PrefsVertButton,
        prefs.numVert - MinVert);
    lstPopSet(alertP, PrefsLocaleList, PrefsLocaleButton,
        prefs.locale / EnLocale);

    // change prefs if OK
    if (PrefsOKButton == FrmDoDialog(alertP)) {
        prefs.numPlayers = MinPlayers +
            ctlGrpSel(alertP, PrefsPlayerBaseButton,
                MaxPlayers - MinPlayers + 1);
        mustInit |= oldNumPlayers != prefs.numPlayers;
        
        for (i = 0; i < MaxPlayers; ++i) {
            prefs.playerType[i] = LstGetSelection(
                getLstPtr(alertP, PrefsTypeBaseList + i*PrefsTypeDelta));
            mustInit |= oldPlayerType[i] != prefs.playerType[i];
        }
    
        prefs.numHoriz = LstGetSelection(
            getLstPtr(alertP, PrefsHorizList))
            + MinHoriz;
        mustInit |= oldNumHoriz != prefs.numHoriz;

        prefs.numVert = LstGetSelection(
            getLstPtr(alertP, PrefsVertList))
            + MinVert;
        mustInit |= oldNumVert != prefs.numVert;

        prefs.locale = LstGetSelection(
            getLstPtr(alertP, PrefsLocaleList))
            * EnLocale;

        if (mustInit)
            init();
            
        changeLocale(frmP);
        refresh(frmP);
    }
    FrmDeleteForm(alertP);
}

static void showAbout() {
    // init form to current values
    FormPtr alertP = FrmInitForm(
        AboutForm + prefs.locale);
    CtlSetLabel(getCtlPtr(alertP,
        AboutVersionLabel), versionP);
    do {
        switch (FrmDoDialog(alertP)) {
            case AboutCreditsButton:
                FrmHelp(AboutCreditsStr + prefs.locale);
                continue;
            case AboutLicenseButton:
                FrmHelp(AboutLicenseStr + prefs.locale);
                continue;
        }
        break;
    } while (true);
    FrmDeleteForm(alertP);
}

static void mainFormInit(FormPtr frmP) {
    inMenu = false;
    changeLocale(frmP);
    refresh(frmP);
}

static Boolean doMenu(FormPtr frmP, UInt16 command) {
    Boolean handled = false;
    switch (command) {
    case NewMenuOpt :
        init();
        refresh(frmP);
        handled = true;
        break;
    case PrefsMenuOpt :
        showPrefs(frmP);
        handled = true;
        break;
    case HowtoMenuOpt :
        FrmHelp(HowtoStr + prefs.locale);
        handled = true;
        break;
    case AboutMenuOpt :
        showAbout();
        handled = true;
        break;
    }
    return handled;
}

Boolean mainFormEventHandler(EventPtr eventP) {
	int b;
    Boolean handled = false;
    FormPtr frmP = FrmGetActiveForm();
    switch (eventP->eType) {
        case frmOpenEvent:
            FrmDrawForm(frmP);
            mainFormInit(frmP);
            handled = true;
            break;
        case winEnterEvent:
            if (eventP->data.winEnter.enterWindow
                == (WinHandle) FrmGetFormPtr(MainForm))
                inMenu = false;
            else
                inMenu = true;
            break;                  
        case menuEvent:
            handled = doMenu(frmP, eventP->data.menu.itemID);
            break;                  
        case ctlSelectEvent:
            b = eventP->data.ctlSelect.controlID;
            if (NewGameButton == b) {
                init();
                CtlHideControl(getCtlPtr(frmP,
                    NewGameButton));        
                refresh(frmP);
            }
            handled = true;
            break;                  
        case penDownEvent:
            if ((eventP->screenY >= homeY)
                && (Invalid != prefs.currPlayer)
                && (TypeHuman == prefs.playerType[prefs.currPlayer]))
            {
                startX = eventP->screenX;
                startY = eventP->screenY;
            } else {
                startY = Invalid;
            }
            break;
        case penMoveEvent:
            if ((eventP->screenY >= homeY)
                && (Invalid != prefs.currPlayer)
                && (TypeHuman == prefs.playerType[prefs.currPlayer]))
            {
                drawBmp(PenBaseBmp + prefs.currPlayer,
                    eventP->screenX, eventP->screenY);
            }
            break;
        case penUpEvent:
            if ((checkLine(frmP, startX, startY,
                    eventP->screenX, eventP->screenY))
                && (Invalid != prefs.currPlayer))
            {
                SndPlaySystemSound(sndClick);
            }
            else {
                SndPlaySystemSound(sndWarning);
            }
            if (Invalid != startY)
                refresh(frmP);
            break;
        case nilEvent:
            if (!inMenu
                && (Invalid != prefs.currPlayer)
                && (TypeHuman != prefs.playerType[prefs.currPlayer]))
            {
                autoPlay(frmP);
            }
            break;
    }
    return handled;
}

int getInitLocale() {
    FormPtr alertP = FrmInitForm(LocaleForm);
    int locale = FrmDoDialog(alertP) - EnLocale;
    FrmDeleteForm(alertP);    
    return (locale > 0) ? locale : EsLocale;
}

void firstRun() {
	int i;
    // actual preferences
    prefs.numPlayers = MinPlayers;
    prefs.numHoriz = FirstHoriz;
    prefs.numVert = FirstVert;
    prefs.locale = EsLocale;
    prefs.playerType[0] = TypeHuman;        
    for (i = 1; i < MaxPlayers; ++i)
        prefs.playerType[i] = TypeMedium;        

    // locale
    prefs.locale = getInitLocale();

    // game state
    init();
    
    // ego
    showAbout();
}

void startApp() {
    UInt16 prefSize;
    oneSecond = SysTicksPerSecond();

    // get version
    copyStrFromRes(versionP, 'tver', VersionRes);
    
    // get format for scores
    copyStrFromRes(scoreFmtP, 'tSTR', ScoreFormatStr);

    // get or init preferences
    prefSize = sizeof(Prefs);
    // default initialization, since discovered Prefs was missing or old
    if ((PrefGetAppPreferences (AppCreator, 1000, &prefs, &prefSize, true) == noPreferenceFound) 
        || (prefSize != sizeof(Prefs)))
        firstRun();
}

void stopApp() {
    // store state
    PrefSetAppPreferences(AppCreator, 1000, 1, &prefs, sizeof(Prefs), true);
}

Boolean appHandleEvent(EventPtr event) {
    FormPtr frmP;
    int formId;
    Boolean handled = false;
    if (event->eType == frmLoadEvent) {
        formId = event->data.frmLoad.formID;
        frmP = FrmInitForm(formId);
        FrmSetActiveForm(frmP);
        if (MainForm == formId)
            FrmSetEventHandler (frmP,
                mainFormEventHandler);
        handled = true;
    }       
    return handled;
}

UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags) {
    EventType event;
    UInt16 error;
    if (cmd == sysAppLaunchCmdNormalLaunch) {
        if (isCompatible()) {
            startApp();
            FrmGotoForm(MainForm);
            do {
                EvtGetEvent(&event,
                    oneSecond / RefreshPerSec);
                if (!SysHandleEvent(&event))
                if (!MenuHandleEvent(0, &event, &error))
                if (!appHandleEvent(&event))
                    FrmDispatchEvent(&event);
             } while (event.eType != appStopEvent);
            stopApp();
        }
        else {
            FrmAlert(
                IncompatAlert + prefs.locale);
        }
        FrmCloseAllForms();
    }
    return 0;
}
