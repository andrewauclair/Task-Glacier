package packets;

public class RequestID {
    private static int requestID = 1;

    public static int nextRequestID() {
        int id = requestID;
        requestID++;
        return id;
    }
}
