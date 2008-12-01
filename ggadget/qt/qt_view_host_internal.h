/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_QT_QT_VIEW_HOST_INTERNAL_H__
#define GGADGET_QT_QT_VIEW_HOST_INTERNAL_H__

namespace ggadget {
namespace qt {

class QtViewHost::Impl : public QObject {
  Q_OBJECT
 public:
  Impl(QtViewHost *owner,
       ViewHostInterface::Type type,
       double zoom,
       QtViewHost::Flags flags,
       int debug_mode,
       QWidget *parent)
    : owner_(owner),
      view_(NULL),
      type_(type),
      widget_(NULL),
      window_(NULL),
      dialog_(NULL),
      debug_mode_(debug_mode),
      zoom_(zoom),
      onoptionchanged_connection_(NULL),
      feedback_handler_(NULL),
      record_states_(flags.testFlag(QtViewHost::FLAG_RECORD_STATES)),
      input_shape_mask_(false),
      keep_above_(false),
      flags_(QtViewWidget::FLAG_MOVABLE | QtViewWidget::FLAG_INPUT_MASK),
      parent_widget_(parent) {
    if (flags.testFlag(QtViewHost::FLAG_WM_DECORATED))
      flags_ |= QtViewWidget::FLAG_WM_DECORATED;

    if (flags.testFlag(QtViewHost::FLAG_COMPOSITE) &&
        type != ViewHostInterface::VIEW_HOST_MAIN)
      flags_ |= QtViewWidget::FLAG_COMPOSITE;
  }

  ~Impl() {
    if (onoptionchanged_connection_)
      onoptionchanged_connection_->Disconnect();

    Detach();
  }

  void Detach() {
    SaveWindowStates();
    view_ = NULL;
    delete window_;
    delete dialog_;
    window_ = widget_ = NULL;
    dialog_ = NULL;
    if (feedback_handler_) {
      delete feedback_handler_;
      feedback_handler_ = NULL;
    }
  }

  std::string GetViewPositionOptionPrefix() {
    switch (type_) {
      case ViewHostInterface::VIEW_HOST_MAIN:
        return "main_view";
      case ViewHostInterface::VIEW_HOST_OPTIONS:
        return "flags_view";
      case ViewHostInterface::VIEW_HOST_DETAILS:
        return "details_view";
      default:
        return "view";
    }
    return "";
  }

  void SaveWindowStates() {
    if (view_ && view_->GetGadget() && record_states_ && window_) {
      OptionsInterface *opt = view_->GetGadget()->GetOptions();
      std::string opt_prefix = GetViewPositionOptionPrefix();
      DLOG("Save:%d, %d", window_->pos().x(), window_->pos().y());
      opt->PutInternalValue((opt_prefix + "_x").c_str(),
                            Variant(window_->pos().x()));
      opt->PutInternalValue((opt_prefix + "_y").c_str(),
                            Variant(window_->pos().y()));
      opt->PutInternalValue((opt_prefix + "_keep_above").c_str(),
                            Variant(keep_above_));
    }
  }

  void DefaultPosition() {
    if (!parent_widget_) return;
    int w = static_cast<int>(view_->GetWidth());
    int h = static_cast<int>(view_->GetHeight());
    QPoint p = GetPopupPosition(parent_widget_->geometry(), QSize(w, h));
    window_->move(p);
  }

  void LoadWindowStates() {
    if (view_ && view_->GetGadget() && record_states_ && window_) {
      OptionsInterface *opt = view_->GetGadget()->GetOptions();
      std::string opt_prefix = GetViewPositionOptionPrefix();
      // restore KeepAbove state
      Variant keep_above =
          opt->GetInternalValue((opt_prefix + "_keep_above").c_str());
      if (keep_above.type() == Variant::TYPE_BOOL &&
          VariantValue<bool>()(keep_above)) {
        KeepAboveMenuCallback(NULL, true);
      }
      // restore position
      Variant vx = opt->GetInternalValue((opt_prefix + "_x").c_str());
      Variant vy = opt->GetInternalValue((opt_prefix + "_y").c_str());
      int x, y;
      if (vx.ConvertToInt(&x) && vy.ConvertToInt(&y)) {
        DLOG("Restore:%d, %d", x, y);
        window_->move(x, y);
        return;
      }
    }
    DefaultPosition();
  }

  bool ShowView(bool modal, int flags,
                Slot1<bool, int> *feedback_handler) {
    ASSERT(view_);
    if (feedback_handler_ && feedback_handler_ != feedback_handler)
      delete feedback_handler_;
    feedback_handler_ = feedback_handler;

    if (widget_) {
      // we just move existing widget_ to the front
      widget_->hide();
      widget_->show();
      return true;
    }

    widget_ = new QtViewWidget(view_, flags_);
    if (type_ == ViewHostInterface::VIEW_HOST_OPTIONS) {
      QVBoxLayout *layout = new QVBoxLayout();
      widget_->setFixedSize(D2I(view_->GetWidth()), D2I(view_->GetHeight()));
      layout->addWidget(widget_);
      ASSERT(!dialog_);
      dialog_ = new QDialog();

      QDialogButtonBox::StandardButtons what_buttons = 0;
      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_OK)
        what_buttons |= QDialogButtonBox::Ok;

      if (flags & ViewInterface::OPTIONS_VIEW_FLAG_CANCEL)
        what_buttons |= QDialogButtonBox::Cancel;

      if (what_buttons != 0) {
        QDialogButtonBox *buttons = new QDialogButtonBox(what_buttons);
        if (flags & ViewInterface::OPTIONS_VIEW_FLAG_OK)
          dialog_->connect(buttons, SIGNAL(accepted()),
                           this, SLOT(OnOptionViewOK()));
        if (flags & ViewInterface::OPTIONS_VIEW_FLAG_CANCEL)
          dialog_->connect(buttons, SIGNAL(rejected()),
                           this, SLOT(OnOptionViewCancel()));
        layout->addWidget(buttons);
      }

      dialog_->setLayout(layout);
      dialog_->setWindowTitle(caption_);
      SetGadgetWindowIcon(dialog_, view_->GetGadget());

      if (modal) {
        dialog_->exec();
      } else {
        dialog_->show();
      }
    } else {
      window_ = widget_;
      SetGadgetWindowIcon(window_, view_->GetGadget());
      window_->setWindowTitle(caption_);

      LoadWindowStates();
      window_->setAttribute(Qt::WA_DeleteOnClose, true);

      if (type_ == ViewHostInterface::VIEW_HOST_MAIN) {
        widget_->EnableInputShapeMask(input_shape_mask_);
      }
      widget_->connect(widget_, SIGNAL(destroyed(QObject*)),
                       this, SLOT(OnViewWidgetClose(QObject*)));
      window_->show();
    }
    return true;
  }

  void KeepAboveMenuCallback(const char *, bool keep_above) {
    if (keep_above_ != keep_above) {
      keep_above_ = keep_above;
      if (window_) {
        widget_->SetKeepAbove(keep_above);
      }
    }
  }

  bool ShowContextMenu(int button) {
    ASSERT(view_);
    context_menu_.clear();
    QtMenu qt_menu(&context_menu_);
    if (view_->OnAddContextMenuItems(&qt_menu) &&
        type_ == VIEW_HOST_MAIN) {
      qt_menu.AddItem(
          GM_("MENU_ITEM_ALWAYS_ON_TOP"),
          keep_above_ ? MenuInterface::MENU_ITEM_FLAG_CHECKED : 0, 0,
          NewSlot(this, &Impl::KeepAboveMenuCallback, !keep_above_),
          MenuInterface::MENU_ITEM_PRI_HOST);
    }

    if (!context_menu_.isEmpty()) {
      context_menu_.popup(QCursor::pos());
      return true;
    } else {
      return false;
    }
  }

  void HandleOptionViewResponse(ViewInterface::OptionsViewFlags flag) {
    if (feedback_handler_) {
      // TODO(idlecat511): Handle result of feedback_handler_ in case of OK.
      (*feedback_handler_)(flag);
      delete feedback_handler_;
      feedback_handler_ = NULL;
    }
    dialog_->hide();
  }

  void HandleDetailsViewClose() {
    if (feedback_handler_) {
      (*feedback_handler_)(ViewInterface::DETAILS_VIEW_FLAG_NONE);
      delete feedback_handler_;
      feedback_handler_ = NULL;
    }
  }

  void SetVisibility(bool flag) {
    if (!window_) return;
    if (flag) {
      widget_->hide();
      widget_->show();
      widget_->SkipTaskBar();
      LoadWindowStates();
    } else {
      SaveWindowStates();
      widget_->hide();
    }
  }

  QtViewHost *owner_;
  ViewInterface *view_;
  ViewHostInterface::Type type_;
  QtViewWidget *widget_;
  QWidget *window_;     // Top level window of the view
  QDialog *dialog_;     // Top level window of the view
  int debug_mode_;
  double zoom_;
  Connection *onoptionchanged_connection_;

  Slot1<bool, int> *feedback_handler_;

  bool record_states_;
  bool input_shape_mask_;
  bool keep_above_;
  QtViewWidget::Flags flags_;
  QWidget *parent_widget_;
  QString caption_;
  QMenu context_menu_;

 public slots:
  void OnOptionViewOK() {
    HandleOptionViewResponse(ViewInterface::OPTIONS_VIEW_FLAG_OK);
  }

  void OnOptionViewCancel() {
    HandleOptionViewResponse(ViewInterface::OPTIONS_VIEW_FLAG_CANCEL);
  }

  void OnViewWidgetClose(QObject*) {
    if (type_ == ViewHostInterface::VIEW_HOST_DETAILS)
      HandleDetailsViewClose();
    window_ = NULL;
    // Quick and dirty hack:
    // user can close a view through close button provided by gadget or through
    // ways provided by windows system. The latter will come here without
    // calling CloseView. So call it here manually.
    // owner_->widget_ is NULL if we come here through CloseView
    if (widget_) {
      widget_ = NULL;
      owner_->CloseView();
    }
  }

  void OnShow(bool flag) {
    SetVisibility(flag);
  }
};

} // namespace qt
} // namespace ggadget
#endif
