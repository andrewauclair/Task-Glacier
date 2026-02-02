package data;

import packets.TaskInfo;

import javax.swing.*;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;

import static taskglacier.MainFrame.mainFrame;

public class TaskModel {
    private List<Task> tasks = new ArrayList<>();
    private List<Listener> listeners = new ArrayList<>();

    public List<Task> getTasks() {
        return Collections.unmodifiableList(tasks);
    }

    public Task getTask(int taskID) {
        return tasks.stream().filter(task -> task.id == taskID)
                .findFirst()
                .orElse(null);
    }

    public Optional<Integer> getActiveTaskID() {
        return tasks.stream().filter(task -> task.state == TaskState.ACTIVE)
                .map(task -> task.id)
                .findFirst();
    }

    public boolean taskHasNonFinishedChildren(int taskID) {
        Optional<Task> optional = tasks.stream()
                .filter(task -> task.id == taskID)
                .findFirst();

        return optional.map(value -> value.children.stream()
                .anyMatch(task -> task.state != TaskState.FINISHED)).orElse(false);
    }

    public void addListener(Listener listener) {
        listeners.add(listener);

        // send all tasks to new listeners
        SwingUtilities.invokeLater(() -> {
            List<Task> tasks = new ArrayList<>();

            for (Task task : this.tasks) {
                if (task.parentID == 0) {
                    tasks.add(task);
                }
            }

            while (!tasks.isEmpty()) {
                List<Task> next = new ArrayList<>();

                for (Task task : tasks) {
                    listener.addTask(task);

                    for (Task task1 : this.tasks) {
                        if (task1.parentID == task.id) {
                            next.add(task1);
                        }
                    }
                }
                tasks = next;
            }
        });
    }

    public void removeListener(Listener listener) {
        listeners.remove(listener);
    }

    public boolean hasTask(int taskID) {
        return tasks.stream().anyMatch(task -> task.id == taskID);
    }

    public void receiveInfo(TaskInfo info) {
        Task task;
        boolean newTask = false;

        Optional<Task> first = tasks.stream().filter(existingTask -> existingTask.id == info.taskID)
                .findFirst();

        if (first.isPresent()) {
            task = first.get();
        }
        else {
            task = new Task(info.taskID, info.parentID, info.name);
            newTask = true;
        }


        task.indexInParent = info.indexInParent;
        task.serverControlled = info.serverControlled;
        task.locked = info.locked;

        task.name = info.name;
        task.state = info.state;
        task.createTime = info.createTime;
        task.sessions = new ArrayList<>(info.sessions);
        task.labels = new ArrayList<>(info.labels);
        task.timeEntry = new ArrayList<>(info.timeEntry);

        if (newTask) {
            tasks.add(task);

            Optional<Task> optionalParent = tasks.stream().filter(parent -> parent.id == info.parentID)
                    .findFirst();

            if (optionalParent.isPresent()) {
                optionalParent.get().children.add(task);
                task.parent = optionalParent.get();
            }
            listeners.forEach(listener -> listener.addTask(task));
        }
        else {
            boolean parentChanged = first.get().parentID != info.parentID;

            if (parentChanged) {
                listeners.forEach(listener -> listener.removeTask(task));
            }

            Optional<Task> optionalOldParent = tasks.stream().filter(parent -> parent.id == task.parentID)
                    .findFirst();

            Optional<Task> optionalNewParent = tasks.stream().filter(parent -> parent.id == info.parentID)
                    .findFirst();

            if (optionalOldParent.isPresent()) {
                optionalOldParent.get().children.remove(task);
                task.parent = null;
            }

            if (optionalNewParent.isPresent()) {
                optionalNewParent.get().children.add(task);
                task.parent = optionalNewParent.get();
            }

            int oldParent = task.parentID;

            task.parentID = info.parentID;

            if (parentChanged) {
                listeners.forEach(listener -> listener.addTask(task));

                List<Task> tasks = new ArrayList<>();

                for (Task child : task.children) {
                    tasks.add(child);
                }

                while (!tasks.isEmpty()) {
                    List<Task> next = new ArrayList<>();

                    for (Task child : tasks) {
                        listeners.forEach(listener -> listener.addTask(child));

                        for (Task children : child.children) {
                            next.add(children);
                        }
                    }
                    tasks = next;
                }
            }
            else {
                listeners.forEach(listener -> listener.updatedTask(first.get()));
            }
        }
    }

    // it's possible that we receive data out-of-order the first time. for updates for everything
    public void configurationComplete() {
        forceUpdates();
        listeners.forEach(Listener::configComplete);
    }

    private void forceUpdates() {
        for (int i = 0; i < tasks.size() + 1; i++) {
            final int parentID = i;
            List<Task> toUpdate = tasks.stream()
                    .filter(t -> t.parentID == parentID)
                    .toList();

            for (Task task1 : toUpdate) {
                listeners.forEach(listener -> listener.updatedTask(task1));
            }
        }
    }

    public void removeUnspecifiedTask() {
        Optional<Task> unspecified = tasks.stream().filter(task -> task.id == 0).findFirst();

        if (unspecified.isPresent()) {
            listeners.forEach(listener -> listener.removeTask(unspecified.get()));
        }
    }

    public void clear() {
        for (Task task : tasks) {
            listeners.forEach(listener -> listener.removeTask(task));
        }
        tasks.clear();
    }

    public interface Listener {
        void addTask(Task task);
        void updatedTask(Task task);
        void removeTask(Task task);
        void configComplete();
    }
}
