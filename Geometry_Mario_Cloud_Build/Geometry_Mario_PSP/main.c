#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <math.h>
#include <string.h>

PSP_MODULE_INFO("Geometry Mario", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define SCREEN_W 480
#define SCREEN_H 272
#define BUF_W 512
#define MAX_COINS 12

static unsigned int __attribute__((aligned(16))) list[262144];
static unsigned int __attribute__((aligned(16))) fb0[BUF_W * SCREEN_H];
static unsigned int __attribute__((aligned(16))) fb1[BUF_W * SCREEN_H];

typedef struct { unsigned int color; short x, y, z; } Vertex;
static Vertex verts[6];

static float player_x, player_y, vy, camera_x;
static int grounded, score, lives, state, frame_count;
static int coin_taken[MAX_COINS];
static int enemy_alive = 1;

static void rect(float x, float y, float w, float h, unsigned int color) {
    verts[0]=(Vertex){color,(short)x,(short)y,0};
    verts[1]=(Vertex){color,(short)(x+w),(short)y,0};
    verts[2]=(Vertex){color,(short)x,(short)(y+h),0};
    verts[3]=(Vertex){color,(short)(x+w),(short)y,0};
    verts[4]=(Vertex){color,(short)(x+w),(short)(y+h),0};
    verts[5]=(Vertex){color,(short)x,(short)(y+h),0};
    sceGuDrawArray(GU_TRIANGLES, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D,
                   6, 0, verts);
}

static void text(int x, int y, const char *s) {
    pspDebugScreenSetXY(x, y);
    pspDebugScreenPrintf("%s", s);
}

static void reset_game(void) {
    player_x=48; player_y=176; vy=0; grounded=0;
    camera_x=0; score=0; lives=3; state=1; frame_count=0;
    enemy_alive=1;
    memset(coin_taken,0,sizeof(coin_taken));
}

static int overlap(float ax,float ay,float aw,float ah,float bx,float by,float bw,float bh) {
    return ax < bx+bw && ax+aw > bx && ay < by+bh && ay+ah > by;
}

static void draw_world(void) {
    int i;
    /* sky and distant hills */
    rect(0,0,SCREEN_W,SCREEN_H,0xFFB86A35);
    for(i=0;i<6;i++) {
        float hx = (i*150) - fmodf(camera_x*0.25f,150.0f);
        rect(hx,155,90,28,0xFF58B85A);
        rect(hx+18,139,48,24,0xFF58B85A);
    }
    /* ground */
    rect(0,224,SCREEN_W,48,0xFF397D2B);
    rect(0,224,SCREEN_W,7,0xFF79D64A);

    /* platforms / brick blocks in world coordinates */
    for(i=0;i<12;i++) {
        float wx=220+i*145;
        float sx=wx-camera_x;
        if(sx>-50 && sx<SCREEN_W+50) {
            rect(sx,178,48,18,0xFFB86B32);
            rect(sx,178,48,3,0xFFFFC36B);
        }
    }
    /* question blocks */
    for(i=0;i<5;i++) {
        float sx=(390+i*360)-camera_x;
        if(sx>-40 && sx<SCREEN_W+40) {
            rect(sx,126,27,27,0xFF18A9E8);
            rect(sx+4,130,19,19,0xFFFFD84A);
            rect(sx+11,132,5,10,0xFF8B4A20);
            rect(sx+11,144,5,4,0xFF8B4A20);
        }
    }
    /* pipes */
    for(i=0;i<4;i++) {
        float sx=(650+i*520)-camera_x;
        if(sx>-60 && sx<SCREEN_W+60) {
            rect(sx,175,48,49,0xFF168C36);
            rect(sx-5,168,58,13,0xFF27C64A);
            rect(sx+8,183,8,41,0xFF0C6829);
        }
    }
    /* coins */
    for(i=0;i<MAX_COINS;i++) {
        float sx=(290+i*115)-camera_x;
        if(!coin_taken[i] && sx>-20 && sx<SCREEN_W+20) {
            rect(sx,145+(i%3)*-18,10,16,0xFF28DFFF);
            rect(sx+3,148+(i%3)*-18,4,10,0xFFFFF2A0);
        }
    }
    /* enemy */
    if(enemy_alive) {
        float ex=540-camera_x;
        if(ex>-40 && ex<SCREEN_W+40) {
            rect(ex,202,23,22,0xFF8B4ACB);
            rect(ex+4,207,5,5,0xFFFFFFFF);
            rect(ex+14,207,5,5,0xFFFFFFFF);
            rect(ex+5,209,3,3,0xFF111111);
            rect(ex+15,209,3,3,0xFF111111);
        }
    }
    /* player: GD-style cube */
    rect(player_x,player_y,24,24,0xFF28DFFF);
    rect(player_x+4,player_y+4,16,16,0xFF1679C7);
    rect(player_x+6,player_y+7,4,5,0xFFFFFFFF);
    rect(player_x+15,player_y+7,4,5,0xFFFFFFFF);
    rect(player_x+7,player_y+9,2,3,0xFF111111);
    rect(player_x+16,player_y+9,2,3,0xFF111111);
    rect(player_x+8,player_y+17,9,2,0xFFFFFFFF);
}

int main(void) {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    sceGuInit();
    sceGuStart(GU_DIRECT,list);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,BUF_W);
    sceGuDispBuffer(SCREEN_W,SCREEN_H,(void*)0x88000,BUF_W);
    sceGuDepthBuffer((void*)0x110000,BUF_W);
    sceGuOffset(2048-(SCREEN_W/2),2048-(SCREEN_H/2));
    sceGuViewport(2048,2048,SCREEN_W,SCREEN_H);
    sceGuDepthRange(65535,0);
    sceGuScissor(0,0,SCREEN_W,SCREEN_H);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
    pspDebugScreenInit();
    reset_game();

    while(1) {
        SceCtrlData pad;
        sceCtrlReadBufferPositive(&pad,1);
        frame_count++;

        if(pad.Buttons & PSP_CTRL_START) {
            if(state==0) reset_game();
            else if(state==1) state=2;
            else state=1;
        }
        if(state==0) {
            if(pad.Buttons & PSP_CTRL_CROSS) reset_game();
        } else if(state==1) {
            float speed=2.2f;
            if(pad.Buttons & PSP_CTRL_CIRCLE) speed=3.5f;
            if(pad.Buttons & PSP_CTRL_LEFT) player_x-=speed;
            if(pad.Buttons & PSP_CTRL_RIGHT) player_x+=speed;
            if(player_x<10) player_x=10;
            if(player_x>SCREEN_W-34) player_x=SCREEN_W-34;
            if((pad.Buttons & PSP_CTRL_CROSS) && grounded) { vy=-7.8f; grounded=0; }

            vy+=0.34f;
            player_y+=vy;
            if(player_y>=200) { player_y=200; vy=0; grounded=1; }
            if(player_y<0) { player_y=0; vy=0; }

            camera_x += 1.15f;
            if(camera_x>1500) camera_x=0;

            for(int i=0;i<MAX_COINS;i++) {
                float cx=(290+i*115)-camera_x;
                float cy=145+(i%3)*-18;
                if(!coin_taken[i] && overlap(player_x,player_y,24,24,cx,cy,10,16)) {
                    coin_taken[i]=1; score+=10;
                }
            }
            if(enemy_alive && overlap(player_x,player_y,24,24,540-camera_x,202,23,22)) {
                if(vy>0 && player_y<205) { enemy_alive=0; score+=50; vy=-5.0f; }
                else { lives--; player_x=48; player_y=176; vy=0; if(lives<=0) state=0; }
            }
            /* pit-like challenge after the final stretch */
            if(camera_x>1450 && player_x>SCREEN_W-80) {
                score+=100; camera_x=0; player_x=48; memset(coin_taken,0,sizeof(coin_taken)); enemy_alive=1;
            }
        }

        sceGuStart(GU_DIRECT,list);
        sceGuClearColor(0xFFB86A35);
        sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
        sceGuColor(0xFFFFFFFF);
        draw_world();
        sceGuFinish();
        sceGuSync(0,0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();

        pspDebugScreenSetXY(1,1);
        pspDebugScreenPrintf("GEOMETRY MARIO   COINS: %d   LIVES: %d",score/10,lives);
        if(state==2) text(17,8,"PAUSED - START TO RESUME");
        if(state==0) {
            text(16,8,"GAME OVER");
            text(12,10,"PRESS X TO TRY AGAIN");
        }
        if(frame_count==1) {
            /* keep debug text visible after the first frame */
        }
    }
    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
