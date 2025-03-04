package packets;

import data.TimeData;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.List;

public class TimeCategoriesData implements Packet {
    public List<TimeData.TimeCategory> getTimeCategories() {
        return timeCategories;
    }

    List<TimeData.TimeCategory> timeCategories;

    public static TimeCategoriesData parse(DataInputStream input) throws IOException {
        TimeCategoriesData data = new TimeCategoriesData();

        int categoryCount = input.readInt();

        for (int i = 0; i < categoryCount; i++) {
            TimeData.TimeCategory timeCategory = new TimeData.TimeCategory();

            timeCategory.id = input.readInt();

            int chars = input.readShort(); // string length

            byte[] bytes = input.readNBytes(chars);

            timeCategory.name = new String(bytes);

            input.readByte();
            input.readInt();
            input.readByte();

            int codeCount = input.readInt();

            for (int j = 0; j < codeCount; j++) {
                TimeData.TimeCode timeCode = new TimeData.TimeCode();

                timeCode.id = input.readInt();

                chars = input.readShort(); // string length

                bytes = input.readNBytes(chars);

                timeCode.name = new String(bytes);

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
    public void writeToOutput(DataOutputStream output) throws IOException {

    }
}
