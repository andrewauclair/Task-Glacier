package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class RequestDailyReport extends RequestPacket {
    public int month;
    public int day;
    public int year;

    public RequestDailyReport(RequestID requestID) {
        super(requestID);
    }

    @Override
    public PacketType type() {
        return PacketType.REQUEST_DAILY_REPORT;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        super.writeToOutput(output);

        output.writeByte(month);
        output.writeByte(day);
        output.writeShort(year);
    }
}
