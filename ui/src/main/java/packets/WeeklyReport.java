package packets;

import data.TimeData;
import taskglacier.MainFrame;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;

public class WeeklyReport implements Packet {
    public DailyReportMessage.DailyReport[] reports = new DailyReportMessage.DailyReport[7];
    private int requestID;
    private int size = 0;

    public static WeeklyReport parse(DataInputStream input, int size) throws IOException {
        WeeklyReport message = new WeeklyReport();
        message.size = message.size;

        input.readInt(); // packet type
        message.requestID = input.readInt();
        Instant reportTime = Instant.ofEpochMilli(input.readLong());

        for (int i = 0; i < 7; i++) {
            DailyReportMessage.DailyReport dailyReport = new DailyReportMessage.DailyReport();
            message.reports[i] = dailyReport;

            dailyReport.found = input.readBoolean();
            dailyReport.time = reportTime;
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

                    TimeData.TimeCategory category = MainFrame.mainFrame.getTimeData().findTimeCategory(timeCategoryID);

                    if (category == null) {
                        category = new TimeData.TimeCategory();
                        category.name = "Unknown";
                    }

                    TimeData.TimeCode code = MainFrame.mainFrame.getTimeData().findTimeCode(timeCodeID);

                    if (code == null) {
                        code = new TimeData.TimeCode();
                        code.name = "Unknown";
                    }

                    dailyReport.timesPerTimeEntry.put(new TimeData.TimeEntry(category, code), time);
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
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.WEEKLY_REPORT;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {

    }
}
