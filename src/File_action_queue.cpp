#include "File_action_queue.h"
#include "File_action_task.h"
#include <QTimer>
#include <QDebug>

Action_queue::Action_queue(int p_id) {
  id = p_id;
  connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

void Action_queue::add_action(Action *t) {
  //QMutexLocker locker(&access_mutex);
  actions.enqueue(t);
  t->moveToThread(this);
  connect(t, SIGNAL(finished()), this, SLOT(launch_action()));
  emit task_added(t);
  if (!isRunning()) {
    QTimer::singleShot(0, this, SLOT(launch_action()));
    start();
  }
}

QQueue<Action *> Action_queue::get_actions() {
  return actions;
}

void Action_queue::launch_action() {
  //QMutexLocker locker(&access_mutex);
  if (actions.isEmpty()) {
    quit();
    return;
  }
  Action* action = actions.dequeue();
  action->set_queue_id(id);
  //action->set_queue(this);
  QTimer::singleShot(0, action, SLOT(run()));
}

Action_queue::~Action_queue() {
  qDebug() << "~Action_queue";
}
