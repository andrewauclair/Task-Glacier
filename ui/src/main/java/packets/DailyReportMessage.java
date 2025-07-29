package packets;

import data.TimeData;
import taskglacier.MainFrame;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;
import java.time.LocalDate;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class DailyReportMessage implements Packet {
    private int size = 0;
    private int requestID;
    private DailyReport report = null;

    public static DailyReportMessage parse(DataInputStream input, int size) throws IOException {
        DailyReportMessage message = new DailyReportMessage();

        input.readInt(); // packet type
        message.requestID = input.readInt();

        message.report = new DailyReport();
        message.report.time = Instant.ofEpochMilli(input.readLong());
        message.report.found = input.readBoolean();
        message.report.month = input.readByte();
        message.report.day = input.readByte();
        message.report.year = input.readShort();

        if (message.report.found) {
            message.report.startTime = Instant.ofEpochMilli(input.readLong());
            message.report.endTime = Instant.ofEpochMilli(input.readLong());
            message.report.totalTime = Instant.ofEpochMilli(input.readLong());

            int count = input.readInt();

            for (int i = 0; i < count; i++) {
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

                message.report.timesPerTimeEntry.put(new TimeData.TimeEntry(category, code), time);
            }

            count = input.readInt();

            for (int i = 0; i < count; i++) {
                DailyReport.TimePair pair = new DailyReport.TimePair();
                pair.taskID = input.readInt();
                pair.index = input.readInt();

                message.report.times.add(pair);
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
        return PacketType.DAILY_REPORT;
    }

    public DailyReport getReport() {
        return report;
    }

    public boolean isReportFound() {
        return report.found;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
    }

    public static class DailyReport {
        public boolean found;

        public Instant time;

        public int month;
        public int day;
        public int year;

        public List<Integer> tasksCreated;
        public List<Integer> tasksFinished;

        public Instant startTime;
        public Instant endTime;
        public Instant totalTime;

        public Map<TimeData.TimeEntry, Instant> timesPerTimeEntry = new HashMap<>();
        public List<TimePair> times = new ArrayList<>();

        public LocalDate getDate() {
            return LocalDate.of(year, month, day);
        }

        public static class TimePair {
            public int taskID;
            public int index;
        }
    }
}
