package data;

import packets.TaskInfo;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public class TaskModel {
    public List<Task> getTasks() {
        return Collections.unmodifiableList(tasks);
    }

    public Task getTask(int taskID) {
        return tasks.stream().filter(task -> task.id == taskID)
                .findFirst()
                .orElse(null);
    }

    public String getTaskName(int taskID) {
        Optional<String> first = tasks.stream().filter(task -> task.id == taskID)
                .map(task -> task.name)
                .findFirst();

        return first.orElse("");
    }

    public TaskState getTaskState(int taskID) {
        return tasks.stream().filter(task -> task.id == taskID)
                .map(task -> task.state)
                .findFirst()
                .orElse(TaskState.INACTIVE);
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

    public interface Listener {
        void newTask(Task task);
        void updatedTask(Task task, boolean parentChanged);
    }

    private List<Task> tasks = new ArrayList<>();
    private List<Listener> listeners = new ArrayList<>();

    public void addListener(Listener listener) {
        listeners.add(listener);
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

        task.serverControlled = info.serverControlled;
        task.locked = info.locked;

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
            }
            listeners.forEach(listener -> listener.newTask(task));
        }
        else {
            boolean parentChanged = first.get().parentID != info.parentID;

            Optional<Task> optionalOldParent = tasks.stream().filter(parent -> parent.id == task.parentID)
                    .findFirst();

            Optional<Task> optionalNewParent = tasks.stream().filter(parent -> parent.id == info.parentID)
                    .findFirst();

            if (optionalOldParent.isPresent()) {
                optionalOldParent.get().children.remove(task);
            }

            if (optionalNewParent.isPresent()) {
                optionalNewParent.get().children.add(task);
            }

            task.parentID = info.parentID;

            listeners.forEach(listener -> listener.updatedTask(first.get(), parentChanged));
        }
//
//        if (hasTask(info.taskID)) {
//
//                first.get().name = info.name;
//                first.get().state = info.state;
//
//                boolean parentChanged = first.get().parentID != info.parentID;
//                first.get().parentID = info.parentID;
//
//                listeners.forEach(listener -> listener.updatedTask(first.get(), parentChanged));
//            }
//        }
//        else {
//            Task task = new Task(info.taskID, info.parentID, info.name);
//            task.state = info.state;
//            task.createTime = info.createTime;
//            tasks.add(task);
//
//            Optional<Task> first = tasks.stream().filter(parent -> parent.id == info.parentID)
//                    .findFirst();
//
//            if (first.isPresent()) {
//                first.get().children.add(task);
//                listeners.forEach(listener -> listener.updatedTask(first.get(), false));
//            }
//
//            listeners.forEach(listener -> listener.newTask(task));
//        }
    }

    // it's possible that we receive data out-of-order the first time. for updates for everything
    public void configurationComplete() {
        forceUpdates();
    }

    private void forceUpdates() {
        for (int i = 0; i < tasks.size() + 1; i++) {
            final int parentID = i;
            List<Task> toUpdate = tasks.stream()
                    .filter(t -> t.parentID == parentID)
                    .toList();

            for (Task task1 : toUpdate) {
                listeners.forEach(listener -> listener.updatedTask(task1, false));
            }
        }
    }
}
