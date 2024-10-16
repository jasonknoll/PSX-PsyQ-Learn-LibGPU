/*
    Learning the PSX GPU & GTE Libraries
    main.c
    Jason Knoll

    Based on the cube example project from PCSX-Redux
*/


// Callback functions
#include <libetc.h>

// Graphics 
#include <libgpu.h>

// Geometry
#include <libgte.h>

// Allows me to set the ram and stack size
#include <libsn.h>

// Controller
#include <libpad.h>

// Standard C library
#include <stdlib.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

#define OTSIZE 4096
#define SCREEN_Z 512
#define CUBESIZE 196
#define TRIANGLESIZE 64
#define LINELEN 64

// Set RAM and Stack sizes to 2MB and 16KB respectivley
unsigned long __ramsize = 0x00200000;
unsigned long __stacksize = 0x00004000;

typedef struct DB {
    DRAWENV draw;
    DISPENV disp;
    u_long ot[OTSIZE];
    POLY_F4 s[6]; // 6 squares to make a cube
    POLY_F3 s_triangle; // TODO draw a triangle
    LINE_F2 test_line; // TODO draw a line
} DB;

static SVECTOR cube_vertices[] = {
    {-CUBESIZE / 2, -CUBESIZE / 2, -CUBESIZE / 2, 0}, {CUBESIZE / 2, -CUBESIZE / 2, -CUBESIZE / 2, 0}, // Side 1
    {CUBESIZE / 2, CUBESIZE / 2, -CUBESIZE / 2, 0},   {-CUBESIZE / 2, CUBESIZE / 2, -CUBESIZE / 2, 0}, // 2 
    {-CUBESIZE / 2, -CUBESIZE / 2, CUBESIZE / 2, 0},  {CUBESIZE / 2, -CUBESIZE / 2, CUBESIZE / 2, 0}, // 3 
    {CUBESIZE / 2, CUBESIZE / 2, CUBESIZE / 2, 0},    {-CUBESIZE / 2, CUBESIZE / 2, CUBESIZE / 2, 0}, // 4
};

// ?
static int cube_indices[] = {
    0, 1, 2, 3, 1, 5, 6, 2, 5, 4, 7, 6, 4, 0, 3, 7, 4, 5, 1, 0, 6, 7, 3, 2,
};

static SVECTOR tri_vertices[] = {
    
};

static void init_cube(DB *db, CVECTOR *col) {
    size_t i;

    for (i = 0; i < ARRAY_SIZE(db->s); ++i) {
        SetPolyF4(&db->s[i]);
        setRGB0(&db->s[i], col[i].r, col[i].g, col[i].b);
    }
}

static void add_cube(u_long *ot, POLY_F4 *s, MATRIX *transform) {
    long p, otz, flg;
    int nclip;
    size_t i;

    SetRotMatrix(transform);
    SetTransMatrix(transform);

    for (i = 0; i < ARRAY_SIZE(cube_indices); i += 4, ++s) {
        nclip = RotAverageNclip4(&cube_vertices[cube_indices[i + 0]], &cube_vertices[cube_indices[i + 1]],
                                 &cube_vertices[cube_indices[i + 2]], &cube_vertices[cube_indices[i + 3]],
                                 (long *)&s->x0, (long *)&s->x1, (long *)&s->x3, (long *)&s->x2, &p, &otz, &flg);

        if (nclip <= 0) continue;

        // Add primitive to the OT
        if ((otz > 0) && (otz < OTSIZE)) AddPrim(&ot[otz], s);
    }
}

static void init_line(DB *db, CVECTOR *col) {
    SetLineF2(&db->test_line);
    setRGB0(&db->test_line, col->r, col->g, col->b);
}

static void add_line(u_long *ot, LINE_F2 *l) {
    short x0 = 10;
    short y0 = 227;

    short x1 = x0; // vertical line
    short y1 = y0 + LINELEN;

    l->x0 = x0;
    l->y0 = y0;

    l->x1 = x0;
    l->y1 = y1;

    AddPrim(&ot[200], l);
}

int main(void) {
    DB db[2];
    DB *cdb;
    SVECTOR rotation = {0};
    VECTOR translation = {0, 0, (SCREEN_Z * 3) / 2, 0};
    MATRIX transform;
    CVECTOR col[6];
    CVECTOR triangle_color;
    size_t i;

    ResetGraph(0);
    InitGeom();

    SetGraphDebug(0);

    FntLoad(960, 256);
    SetDumpFnt(FntOpen(32, 32, 320, 64, 0, 512));

    SetGeomOffset(320, 240);
    SetGeomScreen(SCREEN_Z);

    SetDefDrawEnv(&db[0].draw, 0, 0, 640, 480);
    SetDefDrawEnv(&db[1].draw, 0, 0, 640, 480);
    SetDefDispEnv(&db[0].disp, 0, 0, 640, 480);
    SetDefDispEnv(&db[1].disp, 0, 0, 640, 480);

    srand(0);

    for (i = 0; i < ARRAY_SIZE(col); ++i) {
        col[i].r = rand();
        col[i].g = rand();
        col[i].b = rand();
    }

    init_cube(&db[0], col);
    init_cube(&db[1], col);


    init_line(&db[0], &col[0]);
    init_line(&db[1], &col[0]);

    // Draw display mask*
    SetDispMask(1);

    // Set draw and disp environments to db[0]
    PutDrawEnv(&db[0].draw);
    PutDispEnv(&db[0].disp);

    for (;;) {
        // Double-buffer swapping
        cdb = (cdb == &db[0]) ? &db[1] : &db[0];

        // Rotation speed
        rotation.vy += 8;
        rotation.vz += 4;

        RotMatrix(&rotation, &transform);
        TransMatrix(&transform, &translation);

        ClearOTagR(cdb->ot, OTSIZE);

        // Adds text to print stream?
        FntPrint("Code compiled using Psy-Q libraries\n\n");
        FntPrint("converted by psyq-obj-parser\n\n");
        FntPrint("Trying to print out different shapes\n\n");
        FntPrint("TODO -- Draw a Square");

        // Add cube to the ordering table OT
        add_cube(cdb->ot, cdb->s, &transform);

        add_line(cdb->ot, &cdb->test_line);

        // Wait for screen to sync before clearing and drawing
        DrawSync(0);
        VSync(0);

        ClearImage(&cdb->draw.clip, 60, 120, 120);

        // Draws primitives in cdb->ot
        DrawOTag(&cdb->ot[OTSIZE - 1]);

        // Draw the print stream
        FntFlush(-1);
    }

    return 0;
}
