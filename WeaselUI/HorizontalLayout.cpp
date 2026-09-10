#include "stdafx.h"
#include "HorizontalLayout.h"

using namespace weasel;

void HorizontalLayout::DoLayout(CDCHandle dc, PDWR pDWR) {
  CSize size;
  int width = offsetX + real_margin_x, height = offsetY + real_margin_y;
  int w = offsetX + real_margin_x;

  /* calc mark_text sizes */
  if ((_style.hilited_mark_color & 0xff000000)) {
    CSize sg;
    if (candidates_count) {
      if (_style.mark_text.empty())
        GetTextSizeDW(L"|", 1, pDWR->pTextFormat, pDWR, &sg);
      else
        GetTextSizeDW(_style.mark_text, _style.mark_text.length(),
                      pDWR->pTextFormat, pDWR, &sg);
    }

    mark_width = sg.cx;
    mark_height = sg.cy;
    if (_style.mark_text.empty()) {
      mark_width = mark_height / 7;
      if (_style.linespacing && _style.baseline)
        mark_width =
            (int)((float)mark_width / ((float)_style.linespacing / 100.0f));
      mark_width = max(mark_width, 6);
    }
    mark_gap = (_style.mark_text.empty()) ? mark_width
                                          : mark_width + _style.hilite_spacing;
  }
  int base_offset = ((_style.hilited_mark_color & 0xff000000)) ? mark_gap : 0;

  // calc page indicator
  CSize pgszl, pgszr;
  if (!IsInlinePreedit()) {
    GetTextSizeDW(pre, pre.length(), pDWR->pPreeditTextFormat, pDWR, &pgszl);
    GetTextSizeDW(next, next.length(), pDWR->pPreeditTextFormat, pDWR, &pgszr);
  }
  bool page_en = (_style.prevpage_color & 0xff000000) &&
                 (_style.nextpage_color & 0xff000000);
  int pgw = page_en ? (pgszl.cx + pgszr.cx + _style.hilite_spacing +
                       _style.hilite_padding_x * 2)
                    : 0;
  int pgh = page_en ? max(pgszl.cy, pgszr.cy) : 0;

  /* Preedit */
  if (!IsInlinePreedit() && !_context.preedit.str.empty()) {
    size = GetPreeditSize(dc, _context.preedit, pDWR->pPreeditTextFormat, pDWR);
    int szx = pgw, szy = max(size.cy, pgh);
    // icon size higher then preedit text
    int yoffset = (STATUS_ICON_SIZE >= szy && ShouldDisplayStatusIcon())
                      ? (STATUS_ICON_SIZE - szy) / 2
                      : 0;
    _preeditRect.SetRect(w, height + yoffset, w + size.cx,
                         height + yoffset + size.cy);
    height += szy + 2 * yoffset + _style.spacing;
    width = max(width, real_margin_x * 2 + size.cx + szx);
    if (ShouldDisplayStatusIcon())
      width += STATUS_ICON_SIZE;
  }

  /* Auxiliary */
  if (!_context.aux.str.empty()) {
    size = GetPreeditSize(dc, _context.aux, pDWR->pPreeditTextFormat, pDWR);
    // icon size higher then auxiliary text
    int yoffset = (STATUS_ICON_SIZE >= size.cy && ShouldDisplayStatusIcon())
                      ? (STATUS_ICON_SIZE - size.cy) / 2
                      : 0;
    _auxiliaryRect.SetRect(w, height + yoffset, w + size.cx,
                           height + yoffset + size.cy);
    height += size.cy + 2 * yoffset + _style.spacing;
    width = max(width, real_margin_x * 2 + size.cx);
  }

  int row_cnt = 0;
  int max_width_of_rows = 0;
  int height_of_rows[MAX_CANDIDATES_COUNT] = {0};    // height of every row
  int row_of_candidate[MAX_CANDIDATES_COUNT] = {0};  // row info of every cand
  int mintop_of_rows[MAX_CANDIDATES_COUNT] = {0};

  // only when there are candidates
  if (candidates_count) {
    w = offsetX + real_margin_x;
    for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
      if (i > 0)
        w += _style.candidate_spacing;
      if (id == i)
        w += base_offset;

      int start_w = w;

      /* 1. Đo kích thước Label (số thứ tự 1., 2...) */
      std::wstring label =
          GetLabelText(labels, i, _style.label_text_format.c_str());
      CSize sizeLabel;
      GetTextSizeDW(label, label.length(), pDWR->pLabelTextFormat, pDWR, &sizeLabel);
      int label_w = sizeLabel.cx * labelFontValid;
      int label_h = sizeLabel.cy;

      /* 2. Đo kích thước Text (Chữ Hán) */
      const std::wstring& text = candidates.at(i).str;
      CSize sizeText;
      GetTextSizeDW(text, text.length(), pDWR->pTextFormat, pDWR, &sizeText);
      int text_w = sizeText.cx * textFontValid;
      int text_h = sizeText.cy;

      /* 3. Đo kích thước Comment (Pinyin) */
      CSize sizeComment(0, 0);
      bool has_comment = false;
      bool cmtFontNotTrans =
          (i == id && (_style.hilited_comment_text_color & 0xff000000)) ||
          (i != id && (_style.comment_text_color & 0xff000000));
      if (!comments.at(i).str.empty() && cmtFontValid && cmtFontNotTrans) {
        const std::wstring& comment = comments.at(i).str;
        GetTextSizeDW(comment, comment.length(), pDWR->pCommentTextFormat, pDWR,
                      &sizeComment);
        has_comment = true;
      }
      int cmt_w = sizeComment.cx * cmtFontValid;
      int cmt_h = has_comment ? sizeComment.cy : 0;

      // Chiều rộng nội dung: lấy theo chữ Hán hoặc Pinyin (cái nào dài hơn)
      int content_w = max(text_w, cmt_w);
      int cand_gap = has_comment ? 2 : 0; // khoảng cách dọc giữa Pinyin và chữ Hán
      int cand_h = cmt_h + cand_gap + text_h;

      // Tổng chiều rộng ứng viên
      int total_cand_w = label_w + (label_w > 0 ? _style.hilite_spacing : 0) + content_w;

      // 4. Định vị Label: Đặt ở tầng dưới, thẳng hàng với chữ Hán
      _candidateLabelRects[i].SetRect(w, height + cmt_h + cand_gap, w + label_w,
                                      height + cmt_h + cand_gap + label_h);
      if (label_w > 0)
        w += label_w + _style.hilite_spacing;

      // 5. Định vị Comment (Pinyin): ĐẶT Ở TẦNG TRÊN, CĂN GIỮA
      int cmt_x = w + (content_w - cmt_w) / 2;
      _candidateCommentRects[i].SetRect(cmt_x, height, cmt_x + cmt_w, height + cmt_h);

      // 6. Định vị Text (Chữ Hán): ĐẶT Ở TẦNG DƯỚI, CĂN GIỮA
      int text_x = w + (content_w - text_w) / 2;
      _candidateTextRects[i].SetRect(text_x, height + cmt_h + cand_gap,
                                     text_x + text_w, height + cmt_h + cand_gap + text_h);

      w = start_w + total_cand_w;

      int base_left = (i == id) ? _candidateLabelRects[i].left - base_offset
                                : _candidateLabelRects[i].left;

      // Xử lý tự động xuống hàng nếu chiều dài vượt quá max_width
      int cand_right = start_w + total_cand_w;
      if (_style.max_width > 0 && (base_left > real_margin_x + offsetX) &&
          (cand_right - offsetX + real_margin_x > _style.max_width)) {
        max_width_of_rows = max(max_width_of_rows, start_w);
        w = offsetX + real_margin_x + (i == id ? base_offset : 0);
        int ofx = w - _candidateLabelRects[i].left;
        int ofy = height_of_rows[row_cnt] + _style.candidate_spacing;

        _candidateLabelRects[i].OffsetRect(ofx, ofy);
        _candidateTextRects[i].OffsetRect(ofx, ofy);
        _candidateCommentRects[i].OffsetRect(ofx, ofy);

        mintop_of_rows[row_cnt] = height;
        height += ofy;
        w += total_cand_w;
        row_cnt++;
        max_width_of_rows = max(max_width_of_rows, w);
      } else {
        max_width_of_rows = max(max_width_of_rows, w);
      }

      mintop_of_rows[row_cnt] = height;
      height_of_rows[row_cnt] = max(height_of_rows[row_cnt], cand_h);
      row_of_candidate[i] = row_cnt;
    }

    // 7. Định vị khung highlight lựa chọn (bao trọn cả Pinyin ở trên và chữ Hán ở dưới)
    for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
      int base_left = (i == id) ? _candidateLabelRects[i].left - base_offset
                                : _candidateLabelRects[i].left;
      int right_edge = max(_candidateTextRects[i].right, _candidateCommentRects[i].right);
      _candidateRects[i].SetRect(base_left, mintop_of_rows[row_of_candidate[i]],
                                 right_edge,
                                 mintop_of_rows[row_of_candidate[i]] +
                                     height_of_rows[row_of_candidate[i]]);

      // Căn giữa theo chiều dọc nếu có ứng viên khác trong cùng hàng cao hơn
      int cand_h = _candidateCommentRects[i].Height() + 2 + _candidateTextRects[i].Height();
      int dy = (height_of_rows[row_of_candidate[i]] - cand_h) / 2;
      if (dy > 0) {
        _candidateLabelRects[i].OffsetRect(0, dy);
        _candidateTextRects[i].OffsetRect(0, dy);
        _candidateCommentRects[i].OffsetRect(0, dy);
      }
    }
    height = mintop_of_rows[row_cnt] + height_of_rows[row_cnt] - offsetY;
    width = max(width, max_width_of_rows);
  } else {
    height -= _style.spacing + offsetY;
    width += _style.hilite_spacing + _style.border;
  }

  width += real_margin_x;
  height += real_margin_y;

  if (candidates_count) {
    width = max(width, _style.min_width);
    height = max(height, _style.min_height);
  }
  if (candidates_count) {
    for (auto i = 0; i < candidates_count && i < MAX_CANDIDATES_COUNT; ++i) {
      // make rightest candidate's rect right the same for better look
      if ((i < candidates_count - 1 &&
           row_of_candidate[i] < row_of_candidate[i + 1]) ||
          (i == candidates_count - 1))
        _candidateRects[i].right = width - real_margin_x;
    }
  }
  _highlightRect = _candidateRects[id];
  UpdateStatusIconLayout(&width, &height);
  _contentSize.SetSize(width + offsetX, height + 2 * offsetY);
  _contentRect.SetRect(0, 0, _contentSize.cx, _contentSize.cy);

  // calc page indicator
  if (page_en && candidates_count && !_style.inline_preedit) {
    int _prex = _contentSize.cx - offsetX - real_margin_x +
                _style.hilite_padding_x - pgw;
    int _prey = (_preeditRect.top + _preeditRect.bottom) / 2 - pgszl.cy / 2;
    _prePageRect.SetRect(_prex, _prey, _prex + pgszl.cx, _prey + pgszl.cy);
    _nextPageRect.SetRect(_prePageRect.right + _style.hilite_spacing, _prey,
                          _prePageRect.right + _style.hilite_spacing + pgszr.cx,
                          _prey + pgszr.cy);
    if (ShouldDisplayStatusIcon()) {
      _prePageRect.OffsetRect(-STATUS_ICON_SIZE, 0);
      _nextPageRect.OffsetRect(-STATUS_ICON_SIZE, 0);
    }
  }

  // prepare temp rect _bgRect for roundinfo calculation
  CopyRect(_bgRect, _contentRect);
  _bgRect.DeflateRect(offsetX + 1, offsetY + 1);
  // prepare round info for single row status, only for single row situation
  _PrepareRoundInfo(dc);
  // readjust for multi rows
  if (row_cnt)  // row_cnt > 0, at least 2 candidates
  {
    _roundInfo[0].IsBottomLeftNeedToRound = false;
    _roundInfo[candidates_count - 1].IsTopRightNeedToRound = false;
    for (auto i = 1; i < candidates_count; i++) {
      _roundInfo[i].Hemispherical = _roundInfo[0].Hemispherical;
      if (row_of_candidate[i] == row_cnt &&
          row_of_candidate[i - 1] == row_cnt - 1)
        _roundInfo[i].IsBottomLeftNeedToRound = true;
      if (row_of_candidate[i] == 0 && row_of_candidate[i + 1] == 1)
        _roundInfo[i].IsTopRightNeedToRound = _style.inline_preedit;
    }
  }
  // truely draw content size calculation
  _contentRect.DeflateRect(offsetX, offsetY);
}
