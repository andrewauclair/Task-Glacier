package data;

import packets.TaskInfo;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public class TaskModel {
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

    public interface Listener {
        void newTask(Task task);
        void updatedTask(Task task);
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
        if (hasTask(info.taskID)) {
            Optional<Task> first = tasks.stream().filter(task -> task.id == info.taskID)
                    .findFirst();

            if (first.isPresent()) {
                first.get().state = info.state;
                listeners.forEach(listener -> listener.updatedTask(first.get()));
            }
        }
        else {
            Task task = new Task(info.taskID, info.parentID, info.name);
            task.state = info.state;
            tasks.add(task);
            listeners.forEach(listener -> listener.newTask(task));
        }
    }
}
