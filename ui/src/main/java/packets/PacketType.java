package packets;

public enum PacketType {
    VERSION_REQUEST(1),
    VERSION(2),

    CREATE_TASK(3),
    MOVE_TASK(4),
    START_TASK(5),
    STOP_TASK(6),
    FINISH_TASK(7),

    SUCCESS_RESPONSE(8),
    FAILURE_RESPONSE(9),

    REQUEST_CONFIGURATION(10),
    REQUEST_CONFIGURATION_COMPLETE(11),

    TASK_INFO(12),

    BUGZILLA_INFO(13),
    BUGZILLA_REFRESH(14);

    private final int value;

    PacketType(int value) {
        this.value = value;
    }

    public int value() {
        return value;
    }

    public static PacketType valueOf(int value) {
        for (PacketType packetType : values()) {
            if (packetType.value == value) {
                return packetType;
            }
        }
        return null;
    }
}
