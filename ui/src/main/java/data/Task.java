package data;

import packets.TaskInfo;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

public class Task {
    public final int id;
    public int parentID;
    public TaskState state = TaskState.INACTIVE;
    public String name;

    public Instant createTime;

    public List<Task> children = new ArrayList<>();

    public List<TaskInfo.TaskTime> times = new ArrayList<>();

    public List<String> labels = new ArrayList<>();
    public List<TimeData.TimeCode> timeCodes = new ArrayList<>();

    public Task(int id, int parentID, String name) {
        this.id = id;
        this.parentID = parentID;
        this.name = name;
    }
}
