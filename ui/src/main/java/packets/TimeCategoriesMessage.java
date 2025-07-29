package packets;

import data.TimeData;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class TimeCategoriesMessage implements Packet {
    private final PacketType packetType;
    public int requestID;
    public TimeCategoryModType type = TimeCategoryModType.UPDATE;
    List<TimeData.TimeCategory> timeCategories = new ArrayList<>();
    private int size = 0;
    public TimeCategoriesMessage(PacketType packetType) {
        this.packetType = packetType;
    }

    public static TimeCategoriesMessage parse(DataInputStream input, int size) throws IOException {
        TimeCategoriesMessage data = new TimeCategoriesMessage(PacketType.valueOf(input.readInt()));

        data.size = size;

        int categoryCount = input.readInt();

        for (int i = 0; i < categoryCount; i++) {
            TimeData.TimeCategory timeCategory = new TimeData.TimeCategory();

            timeCategory.id = input.readInt();

            {
                int chars = input.readShort(); // string length
                byte[] bytes = input.readNBytes(chars);
                timeCategory.name = new String(bytes);
            }

            input.readByte();
            input.readInt();
            input.readByte();

            int codeCount = input.readInt();

            for (int j = 0; j < codeCount; j++) {
                TimeData.TimeCode timeCode = new TimeData.TimeCode();

                timeCode.id = input.readInt();

                {
                    int chars = input.readShort(); // string length
                    byte[] bytes = input.readNBytes(chars);
                    timeCode.name = new String(bytes);
                }

                input.readByte();
                input.readInt();
                input.readByte();

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
