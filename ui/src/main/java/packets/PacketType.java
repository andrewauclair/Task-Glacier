package packets;

public enum PacketType {
    VERSION_REQUEST(1),
    VERSION(2),

    CREATE_TASK(3),
    MOVE_TASK(4),
    START_TASK(5),
    STOP_TASK(6),
    FINISH_TASK(7),
    UPDATE_TASK(15),
    REQUEST_TASK(22),

    SUCCESS_RESPONSE(8),
    FAILURE_RESPONSE(9),

    REQUEST_CONFIGURATION(10),
    REQUEST_CONFIGURATION_COMPLETE(11),

    TASK_INFO(12),

    BUGZILLA_INFO(13),
    BUGZILLA_REFRESH(14),

    DAILY_REPORT(16),
    REQUEST_DAILY_REPORT(17),

    WEEKLY_REPORT(18),
    REQUEST_WEEKLY_REPORT(19),

    SEARCH_REQUEST(20),
    SEARCH_RESULTS(21),

    BACKUP_CONFIGURATION(23),
    BACKUP_PERFORMED(24),
    BACKUP_FAILED(25),

    TIME_CATEGORIES_REQUEST(26),
    TIME_CATEGORIES_DATA(27),
    TIME_CATEGORIES_MODIFY(28),

    START_UNSPECIFIED_TASK(29),
    STOP_UNSPECIFIED_TASK(30),
    UNSPECIFIED_TASK_ACTIVE(38),

    BULK_TASK_UPDATE_START(31),
    BULK_TASK_UPDATE_FINISH(32),

    BULK_TASK_INFO_START(33),
    BULK_TASK_INFO_FINISH(34),

    BULK_TASK_ADD_START(35),
    BULK_TASK_ADD_FINISH(36),

    ERROR_MESSAGE(37);

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
