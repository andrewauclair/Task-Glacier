package data;

import packets.TaskInfo;
import packets.TaskStateChange;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public class TaskModel {
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
        void cleared();
        void newTask(int taskID);
        void updatedTask(int taskID);
    }

    private List<Task> tasks = new ArrayList<>();
    private List<Listener> listeners = new ArrayList<>();

    public void addListener(Listener listener) {
        listeners.add(listener);
    }

    public void removeListener(Listener listener) {
        listeners.remove(listener);
    }

    public void clear() {
        tasks.clear();
        listeners.forEach(Listener::cleared);
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
            }

            listeners.forEach(listener -> listener.updatedTask(info.taskID));
        }
        else {
            tasks.add(new Task(info.taskID, info.name));
            listeners.forEach(listener -> listener.newTask(info.taskID));
        }
    }
}
