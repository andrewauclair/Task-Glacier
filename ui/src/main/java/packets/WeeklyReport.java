package packets;

import data.TimeData;
import taskglacier.MainFrame;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;

public class WeeklyReport implements Packet {
    private int requestID;

    public DailyReportMessage.DailyReport[] reports = new DailyReportMessage.DailyReport[7];

    private int size = 0;

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.WEEKLY_REPORT;
    }

    public static WeeklyReport parse(DataInputStream input, int size) throws IOException {
        WeeklyReport message = new WeeklyReport();
        message.size = message.size;

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
                    int timeCategoryID = input.readInt();
                    int timeCodeID = input.readInt();
                    Instant time = Instant.ofEpochMilli(input.readLong());

                    TimeData.TimeEntry entry = new TimeData.TimeEntry();
                    entry.category = MainFrame.mainFrame.getTimeData().findTimeCategory(timeCategoryID);
                    entry.code = MainFrame.mainFrame.getTimeData().findTimeCode(timeCodeID);

                    dailyReport.timesPerTimeEntry.put(entry, time);
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
