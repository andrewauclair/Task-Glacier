package data;

public class Task {
    public final int id;
    public TaskState state = TaskState.INACTIVE;
    public String name;

    public Task(int id, String name) {
        this.id = id;
        this.name = name;
    }
}
