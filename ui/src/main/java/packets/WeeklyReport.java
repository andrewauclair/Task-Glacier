package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;

public class WeeklyReport implements Packet {
    private int requestID;

    public DailyReportMessage.DailyReport[] reports = new DailyReportMessage.DailyReport[7];

    public static WeeklyReport parse(DataInputStream input) throws IOException {
        WeeklyReport message = new WeeklyReport();

        input.readInt(); // packet type
        message.requestID = input.readInt();

        for (int i = 0; i < 7; i++)
        {
            DailyReportMessage.DailyReport dailyReport = new DailyReportMessage.DailyReport();
            message.reports[i] = dailyReport;

            dailyReport.found = input.readBoolean();
            dailyReport.month = input.readByte();
            dailyReport.day = input.readByte();
            dailyReport.year = input.readShort();

            if (dailyReport.found) {
                dailyReport.startTime = Instant.ofEpochMilli(input.readLong());
                dailyReport.endTime = Instant.ofEpochMilli(input.readLong());
                dailyReport.totalTime = Instant.ofEpochMilli(input.readLong());

                int count = input.readInt();

                for (int j = 0; j < count; j++) {
                    int timeCodeID = input.readInt();
                    Instant time = Instant.ofEpochMilli(input.readLong());

                    dailyReport.timesPerCode.put(timeCodeID, time);
                }

                count = input.readInt();

                for (int j = 0; j < count; j++) {
                    DailyReportMessage.DailyReport.TimePair pair = new DailyReportMessage.DailyReport.TimePair();
                    pair.taskID = input.readInt();
                    pair.index = input.readInt();

                    dailyReport.times.add(pair);
                }
            }
        }

        return message;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {

    }
}
