﻿/**
 * File:   rich_text.h
 * Author: AWTK Develop Team
 * Brief:  rich_text
 *
 * Copyright (c) 2018 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2018-07-28 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#include "tkc/mem.h"
#include "base/timer.h"
#include "tkc/utils.h"
#include "base/image_manager.h"
#include "rich_text/rich_text.h"
#include "rich_text/rich_text_parser.h"

static ret_t rich_text_reset(widget_t* widget) {
  rich_text_t* rich_text = RICH_TEXT(widget);
  return_value_if_fail(rich_text != NULL, RET_BAD_PARAMS);

  if (rich_text->node != NULL) {
    rich_text_node_destroy(rich_text->node);
    rich_text->node = NULL;
  }

  if (rich_text->render_node != NULL) {
    rich_text_render_node_destroy(rich_text->render_node);
    rich_text->render_node = NULL;
  }

  return RET_OK;
}

static ret_t rich_text_on_paint_text(widget_t* widget, canvas_t* c) {
  rich_text_render_node_t* iter = NULL;
  rich_text_t* rich_text = RICH_TEXT(widget);
  return_value_if_fail(widget != NULL && rich_text != NULL && c != NULL, RET_BAD_PARAMS);

  if (widget->w <= rich_text->margin << 1 || widget->h <= rich_text->margin << 1) return RET_OK;
  rect_t r_save;
  rect_t r = rect_init(c->ox, c->oy, widget->w, widget->h);
  r.x += rich_text->margin;
  r.y += rich_text->margin;
  r.w -= rich_text->margin << 1;
  r.h -= rich_text->margin << 1;
  canvas_get_clip_rect(c, &r_save);
  r = rect_intersect(&r, &r_save);
  canvas_set_clip_rect(c, &r);

  int32_t align_h = ALIGN_H_LEFT;
  if (widget->astyle) {
    align_h = style_get_int(widget->astyle, STYLE_ID_TEXT_ALIGN_H, ALIGN_H_LEFT);
  }

  iter = rich_text->render_node;
  while (iter != NULL) {
    r = iter->rect;
    if (r.y > widget->h) {
      break;
    }

    if (align_h == ALIGN_H_CENTER) {
      r.x += iter->align_h_w >> 1;
    } else if (align_h == ALIGN_H_RIGHT) {
      r.x += iter->align_h_w;
    }

    switch (iter->node->type) {
      case RICH_TEXT_TEXT: {
        rect_t cr;
        int32_t i = 0;
        float_t x = r.x;
        wchar_t* text = iter->text;
        int32_t spacing = iter->spacing;
        rich_text_font_t* font = &(iter->node->u.text.font);

        canvas_set_text_color(c, font->color);
        canvas_set_font(c, font->name, font->size);
        canvas_set_text_align(c, ALIGN_H_LEFT, font->align_v);

        for (i = 0; i < iter->size; i++) {
          float_t cw = canvas_measure_text(c, text + i, 1);
          cr.x = x;
          cr.y = r.y;
          cr.h = r.h;
          cr.w = cw + 1;

          canvas_draw_text_in_rect(c, text + i, 1, &cr);
          x += cw;
          if (spacing > 0) {
            if (rich_text_is_flexable_w_char(text[i])) {
              x += iter->flexible_w_char_delta_w;
              spacing -= iter->flexible_w_char_delta_w;
            }
          }
        }
        break;
      }
      case RICH_TEXT_IMAGE: {
        bitmap_t bitmap;
        const char* name = iter->node->u.image.name;
        image_draw_type_t draw_type = iter->node->u.image.draw_type;

        if (widget_load_image(widget, name, &bitmap) == RET_OK) {
          canvas_draw_image_ex(c, &bitmap, draw_type, &r);
        }
        break;
      }
      default:
        break;
    }

    iter = iter->next;
  }
  canvas_set_clip_rect(c, &r_save);

  return RET_OK;
}

static ret_t rich_text_ensure_render_node(widget_t* widget, canvas_t* c) {
  rich_text_t* rich_text = RICH_TEXT(widget);
  return_value_if_fail(widget != NULL && rich_text != NULL, RET_BAD_PARAMS);

  if (rich_text->need_reset) {
    str_t str;
    str_init(&str, widget->text.size * 4 + 1);
    str_from_wstr(&str, widget->text.str);
    rich_text_reset(widget);
    rich_text->node = rich_text_parse(str.str, str.size);
    str_reset(&str);
    rich_text->need_reset = FALSE;
  }

  if (rich_text->render_node != NULL) {
    return RET_OK;
  }

  if (rich_text->node == NULL) {
    return RET_FAIL;
  }

  if (rich_text->render_node == NULL) {
    int32_t w = widget->w;
    int32_t h = widget->h;
    int32_t line_gap = rich_text->line_gap;
    int32_t margin = rich_text->margin;

    rich_text->render_node =
        rich_text_render_node_layout(widget, rich_text->node, c, w, h, margin, line_gap);
  }
  return_value_if_fail(rich_text->render_node != NULL, RET_OOM);

  return RET_OK;
}

static ret_t rich_text_on_paint_self(widget_t* widget, canvas_t* c) {
  if (rich_text_ensure_render_node(widget, c) == RET_OK) {
    return rich_text_on_paint_text(widget, c);
  }

  return RET_OK;
}

static ret_t rich_text_on_event(widget_t* widget, event_t* e) {
  uint16_t type = e->type;
  rich_text_t* rich_text = RICH_TEXT(widget);
  return_value_if_fail(rich_text != NULL, RET_BAD_PARAMS);

  switch (type) {
    case EVT_POINTER_DOWN: {
      break;
    }
    case EVT_POINTER_DOWN_ABORT: {
      break;
    }
    case EVT_POINTER_UP: {
      break;
    }
    case EVT_POINTER_LEAVE:
      break;
    case EVT_POINTER_ENTER:
      break;
    case EVT_RESIZE:
    case EVT_MOVE_RESIZE: {
      rich_text->need_reset = TRUE;
      break;
    }
    default:
      break;
  }

  return RET_OK;
}

static ret_t rich_text_set_prop(widget_t* widget, const char* name, const value_t* v) {
  rich_text_t* rich_text = RICH_TEXT(widget);
  return_value_if_fail(rich_text != NULL && name != NULL && v != NULL, RET_BAD_PARAMS);

  if (tk_str_eq(name, WIDGET_PROP_TEXT)) {
    return rich_text_set_text(widget, value_str(v));
  } else if (tk_str_eq(name, WIDGET_PROP_LINE_GAP)) {
    rich_text->line_gap = value_int(v);
    rich_text->need_reset = TRUE;
    return RET_OK;
  } else if (tk_str_eq(name, WIDGET_PROP_MARGIN)) {
    rich_text->margin = value_int(v);
    rich_text->need_reset = TRUE;
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

static ret_t rich_text_on_destroy(widget_t* widget) {
  return rich_text_reset(widget);
}

static const char* s_rich_text_clone_properties[] = {NULL};
TK_DECL_VTABLE(rich_text) = {.size = sizeof(rich_text_t),
                             .type = "rich_text",
                             .parent = TK_PARENT_VTABLE(widget),
                             .create = rich_text_create,
                             .clone_properties = s_rich_text_clone_properties,
                             .on_event = rich_text_on_event,
                             .set_prop = rich_text_set_prop,
                             .on_destroy = rich_text_on_destroy,
                             .on_paint_self = rich_text_on_paint_self};

widget_t* rich_text_create(widget_t* parent, xy_t x, xy_t y, wh_t w, wh_t h) {
  return widget_create(parent, TK_REF_VTABLE(rich_text), x, y, w, h);
}

ret_t rich_text_set_text(widget_t* widget, const char* text) {
  rich_text_t* rich_text = RICH_TEXT(widget);
  return_value_if_fail(rich_text != NULL, RET_BAD_PARAMS);

  wstr_set_utf8(&(widget->text), text);
  rich_text->need_reset = TRUE;
  return RET_OK;
}

widget_t* rich_text_cast(widget_t* widget) {
  return_value_if_fail(WIDGET_IS_INSTANCE_OF(widget, rich_text), NULL);

  return widget;
}
