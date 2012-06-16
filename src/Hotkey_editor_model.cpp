#include "Hotkey_editor_model.h"
#include <QCoreApplication>

Hotkey_editor_model::Hotkey_editor_model(QList<Hotkey> commands, QString settings_group) :
  QAbstractTableModel()
{
  settings.beginGroup(settings_group);
  foreach (Hotkey h, commands) {
    hotkeys << Hotkey(h.name, settings.value(h.name, h.value).toString());
  }
}

Hotkey_editor_model::~Hotkey_editor_model() {
  settings.endGroup();
}

int Hotkey_editor_model::columnCount(const QModelIndex &) const {
  return 2;
}

int Hotkey_editor_model::rowCount(const QModelIndex &) const {
  return hotkeys.count();
}

QVariant Hotkey_editor_model::data(const QModelIndex &index, int role) const {
  if (role == Qt::DisplayRole) {
    if (index.column() == column_name) {
      return tr(hotkeys[index.row()].name.toAscii().constData());
    } else if (index.column() == column_value) {
      return hotkeys[index.row()].value;
    }
  }
  return QVariant();
}

Qt::ItemFlags Hotkey_editor_model::flags(const QModelIndex &index) const {
  if (index.column() == column_value) {
    return Qt::ItemIsEnabled | Qt::ItemIsEditable;
  }
  return Qt::ItemIsEnabled;
}
