package packets;

import data.TimeData;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class TimeEntryData implements Packet {
    private final PacketType packetType = PacketType.TIME_ENTRY_DATA;
    public int requestID;
    public TimeCategoryModType type = TimeCategoryModType.UPDATE;
    List<TimeData.TimeCategory> timeCategories = new ArrayList<>();
    private int size = 0;

    public static TimeEntryData parse(DataInputStream input, int size) throws IOException {
        TimeEntryData data = new TimeEntryData();

        input.readInt(); // packet type

        data.size = size;

        int categoryCount = input.readInt();

        for (int i = 0; i < categoryCount; i++) {
            TimeData.TimeCategory timeCategory = new TimeData.TimeCategory();

            timeCategory.id = input.readInt();
            timeCategory.name = Packet.parseString(input);

            int codeCount = input.readInt();

            for (int j = 0; j < codeCount; j++) {
                TimeData.TimeCode timeCode = new TimeData.TimeCode();

                timeCode.id = input.readInt();
                timeCode.name = Packet.parseString(input);
                timeCode.archived = input.readByte() != 0;

                timeCategory.timeCodes.add(timeCode);
            }

            data.timeCategories.add(timeCategory);
        }

        return data;
    }

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return packetType;
    }

    public List<TimeData.TimeCategory> getTimeCategories() {
        return timeCategories;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        int size = 20;
        for (TimeData.TimeCategory timeCategory : timeCategories) {
            size += 11 + timeCategory.name.length();
            for (TimeData.TimeCode timeCode : timeCategory.timeCodes) {
                size += 7 + timeCode.name.length();
            }
        }
        output.writeInt(size);
        output.writeInt(packetType.value());
        output.writeInt(requestID);
        output.writeInt(type.ordinal());
        output.writeInt(timeCategories.size());

        for (TimeData.TimeCategory timeCategory : timeCategories) {
            output.writeInt(timeCategory.id);
            output.writeShort(timeCategory.name.length());
            output.write(timeCategory.name.getBytes());
            output.writeByte(0); // archived
            output.writeInt(timeCategory.timeCodes.size());
            for (TimeData.TimeCode timeCode : timeCategory.timeCodes) {
                output.writeInt(timeCode.id);
                output.writeShort(timeCode.name.length());
                output.write(timeCode.name.getBytes());
                output.writeByte(0); // archived
            }

        }
    }
}
