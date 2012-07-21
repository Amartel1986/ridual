#ifndef FILE_ACTION_TASK_H
#define FILE_ACTION_TASK_H

#include <QObject>
#include <QStringList>
#include <QVariant>

class File_action_queue;

enum File_action_type {
  file_action_copy,
  file_action_move,
  file_action_link,
  file_action_delete
};

enum Recursive_fetch_option {
  recursive_fetch_on = 1,
  recursive_fetch_off = 2,
  recursive_fetch_auto = 3
};

enum Link_type {
  link_type_soft_absolute,
  link_type_soft_relative,
  link_type_hard
};

class File_action_state {
public:
  File_action_state() : current_progress(0), total_progress(0), errors_count(0) {}
  double current_progress, total_progress;
  QString current_action;
  int errors_count;
};
Q_DECLARE_METATYPE(File_action_state)

class File_action_task : public QObject {
  Q_OBJECT
public:
  explicit File_action_task(File_action_type p_action_type,
                            QStringList p_target,
                            QString p_destination);

  void run(File_action_queue* p_queue);
  inline File_action_queue* get_queue() { return queue; }

  inline void set_recursive_fetch(Recursive_fetch_option p) {
    recursive_fetch_option = p;
  }
  inline void set_link_type(Link_type p) {
    link_type = p;
  }
  
signals:
  void error(QString message);
  void state_changed(File_action_state state);
  
private:
  File_action_type action_type;
  QStringList target;
  QString destination;
  Recursive_fetch_option recursive_fetch_option;
  Link_type link_type;
  File_action_queue* queue;
};

#endif // FILE_ACTION_TASK_H
