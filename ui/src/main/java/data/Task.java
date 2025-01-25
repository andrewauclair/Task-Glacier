package data;

import java.util.List;

public class Task {
    public final int id;
    public int parentID;
    public TaskState state = TaskState.INACTIVE;
    public String name;

    public List<Task> children;

    public Task(int id, int parentID, String name) {
        this.id = id;
        this.parentID = parentID;
        this.name = name;
    }
}
