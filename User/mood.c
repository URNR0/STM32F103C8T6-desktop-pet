/* mood.c —— 情绪模型实现
 *
 * 每个情绪值都夹在 0~100 之间(用饱和加减, 不会越界)。
 * Mood_Tick() 内部自己计时, 每秒跑一次; 里面的"慢速变化"每 5 秒才动一下,
 * 让饥饿上涨、心情下跌都足够慢, 否则几秒就到底了。
 */
#include "mood.h"
#include "pet.h"        /* Pet_GetFace / Pet_SetFace: 情绪反过来驱动表情 */
#include "delay.h"      /* SysTick_GetTick */
#include "nvstore.h"    /* 掉电记忆: 开机读回、互动后保存 */
#include "oled.h"       /* 进度条显示 */

/* ---------- 可调参数 ---------- */
#define TICK_MS          1000    /* tick 周期(毫秒) */
#define SLOW_EVERY       5       /* 每几个 tick 做一次慢速自然变化 */
#define LOW_MOOD         30      /* 心情低于此值 -> 委屈 */
#define HUNGRY           80      /* 饥饿高于此值 -> 委屈(饿了) */
#define SAD_COOLDOWN_MS  10000   /* 因情绪委屈的冷却, 防止每秒都委屈 */

/* ---------- 情绪状态 ---------- */
static struct {
    uint8_t  mood;           /* 心情 0~100 */
    uint8_t  hunger;         /* 饥饿 0~100 */
    uint8_t  bond;           /* 亲密度 0~100 */
    uint32_t last_tick_ms;   /* 上次 tick 时刻 */
    uint32_t last_sad_ms;    /* 上次因情绪委屈的时刻(冷却用) */
    uint8_t  slow_cnt;       /* 慢速变化计数器 */
    uint8_t  last_bar_mood;  /* 上次画进度条时的心情(用于调试/可忽略) */
    uint8_t  last_bar_hunger;/* 上次画进度条时的饥饿 */
} st;

/* 饱和加法: 结果夹在 0~100 */
static uint8_t clamp100(int16_t v)
{
    if (v < 0)   return 0;
    if (v > 100) return 100;
    return (uint8_t)v;
}

/* 掉电记忆: 把当前三个数值写进 Flash(互动后调用) */
static void Mood_Save(void)
{
    PetSave_t s;
    s.mood   = st.mood;
    s.hunger = st.hunger;
    s.bond   = st.bond;
    NvStore_Save(&s);
}

void Mood_Init(void)
{
    PetSave_t saved;

    /* 掉电记忆: 先尝试从 Flash 读回上次的存档 */
    if (NvStore_Load(&saved)) {
        st.mood   = saved.mood;
        st.hunger = saved.hunger;
        st.bond   = saved.bond;
    } else {
        /* 首次开机: 用默认值 */
        st.mood   = 60;      /* 心情一般 */
        st.hunger = 30;      /* 半饱 */
        st.bond   = 0;       /* 刚认识 */
    }

    st.last_tick_ms = SysTick_GetTick();
    st.last_sad_ms  = 0;
    st.slow_cnt     = 0;
    st.last_bar_mood   = 0xFF;   /* 故意设成不可能值 */
    st.last_bar_hunger = 0xFF;
    Mood_DrawBars();              /* 开机先把进度条画出来 */
}

void Mood_Tick(void)
{
    uint32_t now = SysTick_GetTick();

    /* 还没到 1 秒, 直接返回 */
    if ((uint32_t)(now - st.last_tick_ms) < TICK_MS) return;
    st.last_tick_ms = now;
    st.slow_cnt++;

    /* 慢速自然变化: 每 SLOW_EVERY 秒才动一下 */
    if (st.slow_cnt >= SLOW_EVERY) {
        st.slow_cnt = 0;
        st.hunger = clamp100((int16_t)st.hunger + 1);   /* 越来越饿 */
        st.mood   = clamp100((int16_t)st.mood   - 1);   /* 越来越无聊 */
    }

    /* 表情决策: 心情很低 或 太饿 -> 自动委屈(带冷却, 只在醒着时) */
    if (Pet_GetFace() == PET_FACE_NORMAL &&
        (st.mood < LOW_MOOD || st.hunger > HUNGRY) &&
        (uint32_t)(now - st.last_sad_ms) >= SAD_COOLDOWN_MS) {
        st.last_sad_ms = now;
        Pet_SetFace(PET_FACE_SAD);
    }
}

void Mood_Feed(void)
{
    st.hunger = clamp100((int16_t)st.hunger - 30);   /* 吃饱 */
    st.mood   = clamp100((int16_t)st.mood   + 15);   /* 开心 */
    st.bond   = clamp100((int16_t)st.bond   +  5);   /* 更亲 */
    Pet_SetFace(PET_FACE_HAPPY);
    Mood_Save();                                     /* 记住这次喂食 */
}

void Mood_Play(void)
{
    st.mood = clamp100((int16_t)st.mood + 15);   /* 开心 */
    st.bond = clamp100((int16_t)st.bond +  3);   /* 更亲 */
    Pet_SetFace(PET_FACE_HAPPY);
    Mood_Save();                                 /* 记住这次互动 */
}

uint8_t Mood_GetMood(void)   { return st.mood; }
uint8_t Mood_GetHunger(void) { return st.hunger; }
uint8_t Mood_GetBond(void)   { return st.bond; }

/* ---------- 进度条显示 ---------- */
/* 在屏幕左右两侧画竖直进度条:
 *   左侧(x=0~4):   心情(mood),   从下往上填充
 *   右侧(x=123~127): 饥饿(hunger), 从下往上填充
 * 每条宽 5 像素、高 64 像素, 贴在屏幕最边缘, 对脸的影响最小。
 * 注意: 这个函数只操作显存缓冲区, 不调用 OLED_Refresh(),
 * 由 main 循环统一刷新, 避免表情帧和进度条多次刷新导致闪烁。
 */
void Mood_DrawBars(void)
{
    uint8_t bar_h;    /* 当前进度条填充高度 */

    st.last_bar_mood   = st.mood;
    st.last_bar_hunger = st.hunger;

    /* 清零两侧进度条区域, 避免帧残留 */
    OLED_FillRect(0, 0, 5, 64, 0);
    OLED_FillRect(123, 0, 5, 64, 0);

    /* ===== 左侧: 心情(从下往上填充) ===== */
    OLED_DrawRect(0, 0, 5, 64, 1);           /* 外框 */
    bar_h = (uint8_t)((uint16_t)st.mood * 60 / 100);  /* 内留 2 像素边 */
    if (bar_h) OLED_FillRect(1, 62 - bar_h, 3, bar_h, 1);

    /* ===== 右侧: 饥饿(从下往上填充) ===== */
    OLED_DrawRect(123, 0, 5, 64, 1);         /* 外框 */
    bar_h = (uint8_t)((uint16_t)st.hunger * 60 / 100);
    if (bar_h) OLED_FillRect(124, 62 - bar_h, 3, bar_h, 1);
}
