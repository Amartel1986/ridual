#include "Pane.h"
#include "ui_Pane.h"
#include <QKeyEvent>
#include "Main_window.h"
#include <QMovie>
#include <QDebug>
#include "Directory.h"
#include <QMessageBox>
#include <QMenu>
#include <QScrollBar>

Pane::Pane(QWidget *parent) : QWidget(parent), ui(new Ui::Pane) {
  directory = 0;
  pending_directory = 0;
  ui->setupUi(this);
  ui->list->setModel(&file_list_model);
  ui->list->installEventFilter(this);
  ui->list->viewport()->installEventFilter(this);
  ui->list->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  ui->address->installEventFilter(this);
  ready = true;
  main_window = 0;
  connect(ui->address, SIGNAL(returnPressed()), this, SLOT(on_go_clicked()));
  connect(ui->list, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(open_current()));

  ui->loading_indicator->hide();
  QMovie* loading_movie = new QMovie(":/loading.gif", QByteArray(), ui->loading_indicator);
  ui->loading_indicator->setMovie(loading_movie);
  loading_movie->start();

  connect(ui->list->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(current_index_changed(QModelIndex,QModelIndex)));
}

Pane::~Pane() {
  delete ui;
  if (directory) delete directory;
}

void Pane::set_main_window(Main_window *p_main_window) {
  main_window = p_main_window;
  connect(main_window, SIGNAL(active_pane_changed()), this, SLOT(active_pane_changed()));
}

void Pane::set_uri(QString new_directory) {
  if (directory && new_directory == directory->get_uri()) {
    directory->refresh();
    return;
  }
  if (pending_directory) delete pending_directory;
  pending_directory = new Directory(main_window, new_directory);
  connect(pending_directory, SIGNAL(ready(File_info_list)),
          this, SLOT(directory_ready(File_info_list)));
  connect(pending_directory, SIGNAL(error(QString)),
          this, SLOT(directory_error(QString)));
  pending_directory->refresh();
  //ui->address->setText(directory);

  ready = false;  
  QTimer* timer = new QTimer();
  timer->singleShot(300, this, SLOT(show_loading_indicator()));

}

bool Pane::eventFilter(QObject *object, QEvent *event) {
  //by default using navigation keys sets selection to current row;
  //we need to change active row (dotted border usually) using navigation keys
  //without changing selection. It's the hormal behaviour for ctrl+nav.keys,
  //so we're going to emulate Ctrl pressing.
  QList<int> nav_keys;
  nav_keys << Qt::Key_Down << Qt::Key_Up <<
              Qt::Key_Space <<
              Qt::Key_PageDown << Qt::Key_PageUp << Qt::Key_Home << Qt::Key_End;
  //qDebug() << event;
  if (object == ui->list) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent* key_event = dynamic_cast<QKeyEvent*>(event);
      Q_ASSERT(key_event != 0);
      if (!key_event) return false;
      if (nav_keys.contains(key_event->key()) && key_event->modifiers() == Qt::NoModifier) {
        key_event->setModifiers(Qt::ControlModifier);
      }
      if (key_event->key() == Qt::Key_Left || key_event->key() == Qt::Key_Right) {
        if (key_event->modifiers() == Qt::NoModifier) {
          return true;
        }
      }
      if (key_event->key() == Qt::Key_Return && key_event->modifiers() == Qt::NoModifier) {
        open_current();
        return true;
      }
    } else if (event->type() == QEvent::FocusIn) {
      if (main_window) main_window->set_active_pane(this);
    }
  }
  if (object == ui->address) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent* key_event = dynamic_cast<QKeyEvent*>(event);
      Q_ASSERT(key_event != 0);
      if (!key_event) return false;
      if (key_event->key() == Qt::Key_Escape && key_event->modifiers() == Qt::NoModifier) {
        ui->list->setFocus();
        return true;
      }
    }
  }
  /*if (object == ui->list->viewport()) {
    QMouseEvent* e = dynamic_cast<QMouseEvent*>(event);
    if (e && e->buttons() == Qt::LeftButton) {
       ui->list->selectionModel()->setCurrentIndex(file_list_model.index(
                                                     ui->list->indexAt(e->pos()).row(), 0),
                                                     QItemSelectionModel::NoUpdate);
       return true;
     }
  }*/
  return false;
}

void Pane::load_state(QSettings *s) {
  set_uri(s->value("path", QDir::current().absolutePath()).toString());
}

void Pane::save_state(QSettings *s) {
  s->setValue("path", get_uri());
}

bool Pane::is_active() const {
  if (!main_window) return false;
  return main_window->get_active_pane() == this;
}

QString Pane::get_uri() {
  return directory? directory->get_uri(): QString();
}

void Pane::go_parent() {
  set_uri(directory->get_parent_uri());
}

void Pane::open_current() {
  File_info info = file_list_model.info(ui->list->currentIndex());
  if (info.is_folder()) {
    set_uri(info.uri);
  } else {
    main_window->get_default_app(info.mime_type).launch(info.full_path);
  }
}

void Pane::focus_address_line() {
  ui->address->setFocus();
  ui->address->selectAll();
}

void Pane::refresh() {
  directory->refresh();
}

void Pane::on_go_clicked() {
  ui->list->setFocus();
  set_uri(ui->address->text());
}


void Pane::active_pane_changed() {
  QFont font = ui->address->font();
  font.setBold(is_active());
  ui->address->setFont(font);
  if (is_active()) {
    ui->list->setFocus();
  }
}

void Pane::show_loading_indicator() {
  sender()->deleteLater(); // delete QTimer
  if (pending_directory != 0) {
    ui->loading_indicator->show();
  }
}

void Pane::directory_ready(File_info_list files) {
  bool old_state_stored = false;
  QString new_current_uri;
  QModelIndex old_current_index;
  QItemSelection old_selection;
  QPoint old_scroll_pos;

  if (sender() == pending_directory) {
    if (pending_directory && directory &&
        pending_directory->get_uri() == directory->get_parent_uri()) {
      new_current_uri = directory->get_uri();
    }
    if (directory) delete directory;
    directory = pending_directory;
    emit uri_changed();
    pending_directory = 0;
    ui->loading_indicator->hide();
    ui->address->setText(directory->get_uri());
  } else if (sender() == directory) {
    //it's a refresh, we need to store selection state
    old_state_stored = true;
    old_current_index = ui->list->currentIndex();
    old_selection = ui->list->selectionModel()->selection();
    old_scroll_pos.setX(ui->list->horizontalScrollBar()->value());
    old_scroll_pos.setY(ui->list->verticalScrollBar()->value());
  } else {
    qWarning("Unknown sender");
    return;
  }

  file_list_model.set_data(files);
  if (file_list_model.rowCount() > 0) {
    ui->list->setCurrentIndex(file_list_model.index(0, 0));
  }
  ui->list->clearSelection();
  ui->loading_indicator->hide();

  if (old_state_stored) {
    ui->list->setCurrentIndex(old_current_index);
    ui->list->selectionModel()->select(old_selection, QItemSelectionModel::SelectCurrent);
    ui->list->horizontalScrollBar()->setValue(old_scroll_pos.x());
    ui->list->verticalScrollBar()->setValue(old_scroll_pos.y());
  }
  if (!new_current_uri.isEmpty()) {
    ui->list->selectionModel()->setCurrentIndex(file_list_model.index_for_uri(new_current_uri),
                              QItemSelectionModel::NoUpdate);
  }

  ui->list->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);

}

void Pane::directory_error(QString message) {
  if (sender() == pending_directory) {
    pending_directory->deleteLater();
    pending_directory = 0;
    ui->loading_indicator->hide();

  } else if (sender() == directory) {

  } else {
    qWarning("Unknown sender");
    return;
  }
  //todo: async messages
  QMessageBox::critical(0, tr("Failed to get file list"), message);
}

void Pane::current_index_changed(QModelIndex current, QModelIndex previous) {
  if (current.column() != 0) {
    ui->list->selectionModel()->setCurrentIndex(file_list_model.index(current.row(), 0),
                              QItemSelectionModel::NoUpdate);
  }
}



void Pane::on_list_customContextMenuRequested(const QPoint &pos) {
  File_info file = file_list_model.info(ui->list->indexAt(pos));
  App_info_list apps = main_window->get_apps(file.mime_type);
  QMenu* menu = new QMenu(this);
  if (file.is_folder()) {
    menu->addAction(tr("Browse"))->setEnabled(false);
  }
  if (file.is_file) {
    menu->addAction(tr("View"))->setEnabled(false);
    menu->addAction(tr("Edit"))->setEnabled(false);
  }
  if (file.is_executable) {
    menu->addAction(tr("Execute"))->setEnabled(false);
  }
  if (menu->actions().count() > 0) menu->addSeparator();

  foreach(App_info app, apps) {
    QAction* a = menu->addAction(tr("Open with %1 (%2)").arg(app.name()).arg(app.command()), this, SLOT(action_launch_triggered()));
    QVariantList data;
    data << QVariant::fromValue(app) << QVariant::fromValue(file);
    a->setData(data);
    if (app == apps.default_app) {
      QFont f = a->font();
      f.setBold(true);
      a->setFont(f);
    }
  }
  menu->exec(ui->list->mapToGlobal(pos));
}

void Pane::action_launch_triggered() {
  QAction* a = dynamic_cast<QAction*>(sender());
  if (!a) return;
  QVariantList data = a->data().toList();
  if (data.count() != 2 || !data[0].canConvert<App_info>() || !data[1].canConvert<File_info>()) {
    qWarning("wrong data attached");
    return;
  }
  App_info app = data[0].value<App_info>();
  File_info file = data[1].value<File_info>();
  app.launch(file.full_path);
}
