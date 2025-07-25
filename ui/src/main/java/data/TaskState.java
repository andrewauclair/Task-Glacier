package data;

public enum TaskState {
    PENDING,
    ACTIVE,
    FINISHED;

    public static TaskState valueOf(int value) {
        for (TaskState state : TaskState.values()) {
            if (state.ordinal() == value) {
                return state;
            }
        }
        return null;
    }
}
