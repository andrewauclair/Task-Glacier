package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class DailyReportMessage implements Packet {
    public static class DailyReport {
        public int month;
        public int day;
        public int year;

        public List<Integer> tasksCreated;
        public List<Integer> tasksFinished;

        public Instant startTime;
        public Instant endTime;
        public Instant totalTime;

        public Map<Integer, Instant> timesPerCode = new HashMap<>();

        public static class TimePair {
            public int taskID;
            public int index;
        }
        public List<TimePair> times = new ArrayList<>();
    }

    private int requestID;
    private boolean reportFound;

    public DailyReport getReport() {
        return report;
    }

    public boolean isReportFound() {
        return reportFound;
    }

    private DailyReport report = null;

    public static DailyReportMessage parse(DataInputStream input) throws IOException {
        DailyReportMessage message = new DailyReportMessage();

        input.readInt(); // packet type
        message.requestID = input.readInt();
        message.reportFound = input.readBoolean();

        if (message.reportFound) {
            message.report = new DailyReport();

            message.report.month = input.readByte();
            message.report.day = input.readByte();
            message.report.year = input.readShort();

            message.report.startTime = Instant.ofEpochMilli(input.readLong());
            message.report.endTime = Instant.ofEpochMilli(input.readLong());
            message.report.totalTime = Instant.ofEpochMilli(input.readLong());

            int count = input.readInt();

            for (int i = 0; i < count; i++) {
                int timeCodeID = input.readInt();
                Instant time = Instant.ofEpochMilli(input.readLong());

                message.report.timesPerCode.put(timeCodeID, time);
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
    public void writeToOutput(DataOutputStream output) throws IOException {
    }
}
