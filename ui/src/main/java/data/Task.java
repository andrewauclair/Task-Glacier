package data;

import packets.TaskInfo;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class Task {
    public final int id;
    public Task parent = null;
    public int parentID;
    public TaskState state = TaskState.PENDING;
    public String name;
    public int indexInParent = 0;
    public boolean serverControlled = false;
    public boolean locked = false;

    public Instant createTime;
    public List<Task> children = new ArrayList<>();
    public List<TaskInfo.Session> sessions = new ArrayList<>();
    public List<String> labels = new ArrayList<>();
    public List<TimeData.TimeEntry> timeEntry = new ArrayList<>();
    public Task(int id, int parentID, String name) {
        this.id = id;
        this.parentID = parentID;
        this.name = name;
    }

    public List<Task> getChildren() {
        return children;
    }

    @Override
    public boolean equals(Object o) {
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
        Task task = (Task) o;
        return id == task.id;
    }

    @Override
    public int hashCode() {
        return Objects.hash(id);
    }
}
