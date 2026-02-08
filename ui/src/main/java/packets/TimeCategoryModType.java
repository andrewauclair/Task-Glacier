package packets;

import data.TaskState;

public enum TimeCategoryModType {
    ADD,
    UPDATE;

    public static TimeCategoryModType valueOf(int value) {
        for (TimeCategoryModType state : TimeCategoryModType.values()) {
            if (state.ordinal() == value) {
                return state;
            }
        }
        return null;
    }
}
