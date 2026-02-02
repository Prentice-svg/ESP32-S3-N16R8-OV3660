/**
 * UTF-8 to GB2312 Encoding Conversion
 * Simplified implementation without large lookup table
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "font.h"

#define UTF8_INVALID 0xFFFFFFFFu

/**
 * Decode a single UTF-8 character
 * Returns Unicode code point or UTF8_INVALID on error
 */
static uint32_t decode_utf8_char(const uint8_t **ptr)
{
    if (!ptr || !*ptr) {
        return UTF8_INVALID;
    }

    const uint8_t *s = *ptr;
    uint8_t b0 = *s++;

    if (b0 == 0) {
        return UTF8_INVALID;
    }

    if (b0 < 0x80) {
        *ptr = s;
        return b0;
    }

    if ((b0 & 0xE0) == 0xC0) {
        if (*s == 0) {
            *ptr = s;
            return UTF8_INVALID;
        }
        uint8_t b1 = *s++;
        if ((b1 & 0xC0) != 0x80) {
            *ptr = s;
            return UTF8_INVALID;
        }
        *ptr = s;
        return ((uint32_t)(b0 & 0x1F) << 6) | (b1 & 0x3F);
    }

    if ((b0 & 0xF0) == 0xE0) {
        if (*s == 0) {
            *ptr = s;
            return UTF8_INVALID;
        }
        uint8_t b1 = *s++;
        if ((b1 & 0xC0) != 0x80) {
            *ptr = s;
            return UTF8_INVALID;
        }
        if (*s == 0) {
            *ptr = s;
            return UTF8_INVALID;
        }
        uint8_t b2 = *s++;
        if ((b2 & 0xC0) != 0x80) {
            *ptr = s;
            return UTF8_INVALID;
        }
        *ptr = s;
        return ((uint32_t)(b0 & 0x0F) << 12) |
               ((uint32_t)(b1 & 0x3F) << 6) |
               (b2 & 0x3F);
    }

    if ((b0 & 0xF8) == 0xF0) {
        if (*s == 0) {
            *ptr = s;
            return UTF8_INVALID;
        }
        uint8_t b1 = *s++;
        if ((b1 & 0xC0) != 0x80) {
            *ptr = s;
            return UTF8_INVALID;
        }
        if (*s == 0) {
            *ptr = s;
            return UTF8_INVALID;
        }
        uint8_t b2 = *s++;
        if ((b2 & 0xC0) != 0x80) {
            *ptr = s;
            return UTF8_INVALID;
        }
        if (*s == 0) {
            *ptr = s;
            return UTF8_INVALID;
        }
        uint8_t b3 = *s++;
        if ((b3 & 0xC0) != 0x80) {
            *ptr = s;
            return UTF8_INVALID;
        }
        *ptr = s;
        return ((uint32_t)(b0 & 0x07) << 18) |
               ((uint32_t)(b1 & 0x3F) << 12) |
               ((uint32_t)(b2 & 0x3F) << 6) |
               (b3 & 0x3F);
    }

    *ptr = s;
    return UTF8_INVALID;
}

/**
 * Convert Unicode code point to GB2312
 * For common Chinese characters in our menu
 * Returns true if conversion successful
 */
bool font_unicode_to_gb2312(uint16_t unicode, uint8_t *char_hi, uint8_t *char_lo)
{
    // Direct mapping for common menu characters
    // This is a simplified table for the characters we use

    // Menu characters:
    // 开始延时 停止延时 单次拍摄 系统信息 深度睡眠
    // 延时摄影 已启动 已停止 拍摄中 拍摄完成 已存SD卡
    // 低电量 就绪 WiFi关闭 K1菜单 K2开始
    // 按BOOT唤醒

    static const struct {
        uint16_t unicode;
        uint16_t gb2312;
    } common_map[] = {
        {0x5F00, 0xBFAA}, // 开
        {0x59CB, 0xCABC}, // 始
        {0x5EF6, 0xD1D3}, // 延
        {0x65F6, 0xCAB1}, // 时
        {0x505C, 0xCDA3}, // 停
        {0x6B62, 0xD6B9}, // 止
        {0x5355, 0xB5A5}, // 单
        {0x6B21, 0xB4CE}, // 次
        {0x62CD, 0xC9E3}, // 拍
        {0x6444, 0xC9E3}, // 摄
        {0x7CFB, 0xCFB5}, // 系
        {0x7EDF, 0xCDB3}, // 统
        {0x4FE1, 0xD0C5}, // 信
        {0x606F, 0xA2CF}, // 息
        {0x6DF1, 0xC9EE}, // 深
        {0x5EA6, 0xB6C8}, // 度
        {0x7761, 0xCBAF}, // 睡
        {0x7720, 0xC3DF}, // 眠
        {0x5DF2, 0xD2D1}, // 已
        {0x542F, 0xC6F4}, // 启
        {0x52A8, 0xB6AF}, // 动
        {0x4E2D, 0xD6D0}, // 中
        {0x5B8C, 0xCDAC}, // 完
        {0x6210, 0xB3C9}, // 成
        {0x5B58, 0xB4E6}, // 存
        {0x5361, 0xA1A8}, // 卡
        {0x4F4E, 0xB5CD}, // 低
        {0x7535, 0xB5E7}, // 电
        {0x91CF, 0xC1BF}, // 量
        {0x5C31, 0xBEC7}, // 就
        {0x7EEA, 0xD0F8}, // 绪
        {0x5173, 0xB9D8}, // 关
        {0x95ED, 0xB1D5}, // 闭
        {0x6309, 0xB0B4}, // 按
        {0x5524, 0xBB0D}, // 唤
        {0x9192, 0xD0CF}, // 醒
    };

    for (size_t i = 0; i < sizeof(common_map) / sizeof(common_map[0]); i++) {
        if (common_map[i].unicode == unicode) {
            if (char_hi) {
                *char_hi = (common_map[i].gb2312 >> 8) & 0xFF;
            }
            if (char_lo) {
                *char_lo = common_map[i].gb2312 & 0xFF;
            }
            return true;
        }
    }

    return false;
}

/**
 * Decode next UTF-8 character and convert it to GB2312
 * For GB2312 encoded strings, returns the bytes directly
 */
bool font_utf8_to_gb2312(const uint8_t **utf8_ptr, uint8_t *char_hi, uint8_t *char_lo)
{
    if (!utf8_ptr || !*utf8_ptr) {
        return false;
    }

    const uint8_t *ptr = *utf8_ptr;

    // Check for GB2312 pattern (both bytes 0xA1-0xFE)
    if (ptr[0] >= 0xA1 && ptr[0] <= 0xF7 &&
        ptr[1] >= 0xA1 && ptr[1] <= 0xFE) {
        // Direct GB2312 - return as-is
        *utf8_ptr = ptr + 2;
        if (char_hi) {
            *char_hi = ptr[0];
        }
        if (char_lo) {
            *char_lo = ptr[1];
        }
        return true;
    }

    // UTF-8: decode and convert to GB2312
    uint32_t codepoint = decode_utf8_char(&ptr);
    *utf8_ptr = ptr;

    if (codepoint == UTF8_INVALID || codepoint < 0x80 || codepoint > 0xFFFF) {
        return false;
    }

    return font_unicode_to_gb2312((uint16_t)codepoint, char_hi, char_lo);
}
