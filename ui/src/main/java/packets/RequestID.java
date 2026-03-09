package packets;

public class RequestID {
    private static int requestID = 1;

    public static RequestID nextRequestID() {
        int id = requestID;
        requestID++;
        return new RequestID(id);
    }

    private final int id;

    private RequestID(int id) {
        this.id = id;
    }

    public int getId() {
        return id;
    }
}
