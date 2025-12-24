package packets;

public enum PacketType {
    VERSION_REQUEST(1),
    VERSION(2),

    CREATE_TASK(3),
    MOVE_TASK(4),
    START_TASK(5),
    STOP_TASK(6),
    FINISH_TASK(7),
    UPDATE_TASK(8),
    REQUEST_TASK(9),

    EDIT_TASK_SESSION(10),
    ADD_TASK_SESSION(11),
    REMOVE_TASK_SESSION(12),

    SUCCESS_RESPONSE(13),
    FAILURE_RESPONSE(14),

    REQUEST_CONFIGURATION(15),
    REQUEST_CONFIGURATION_COMPLETE(16),

    TASK_INFO(17),

    BUGZILLA_INFO(18),
    BUGZILLA_REFRESH(19),

    DAILY_REPORT(20),
    REQUEST_DAILY_REPORT(21),

    WEEKLY_REPORT(22),
    REQUEST_WEEKLY_REPORT(23),

    SEARCH_REQUEST(24),
    SEARCH_RESULTS(25),

    BACKUP_CONFIGURATION(26),
    BACKUP_PERFORMED(27),
    BACKUP_FAILED(28),

    TIME_CATEGORIES_REQUEST(29),
    TIME_CATEGORIES_DATA(30),
    TIME_CATEGORIES_MODIFY(31),

    START_UNSPECIFIED_TASK(32),
    STOP_UNSPECIFIED_TASK(33),
    UNSPECIFIED_TASK_ACTIVE(34),

    BULK_TASK_UPDATE_START(35),
    BULK_TASK_UPDATE_FINISH(36),

    BULK_TASK_INFO_START(37),
    BULK_TASK_INFO_FINISH(38),

    BULK_TASK_ADD_START(39),
    BULK_TASK_ADD_FINISH(40),

    ERROR_MESSAGE(41);

    private final int value;

    PacketType(int value) {
        this.value = value;
    }

    public static PacketType valueOf(int value) {
        for (PacketType packetType : values()) {
            if (packetType.value == value) {
                return packetType;
            }
        }
        return null;
    }

    public int value() {
        return value;
    }
}
