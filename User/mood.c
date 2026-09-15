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
