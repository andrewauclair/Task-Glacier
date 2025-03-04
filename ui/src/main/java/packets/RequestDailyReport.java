package packets;

import java.io.DataOutputStream;
import java.io.IOException;

public class RequestDailyReport implements Packet {
    public int requestID;
    public int month;
    public int day;
    public int year;

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        output.writeInt(16);
        output.writeInt(PacketType.REQUEST_DAILY_REPORT.value());
        output.writeInt(requestID);
        output.writeByte(month);
        output.writeByte(day);
        output.writeShort(year);
    }
}
