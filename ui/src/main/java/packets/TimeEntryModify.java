package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class TimeEntryModify extends RequestPacket {
    private final PacketType packetType = PacketType.TIME_ENTRY_MODIFY;

    public static class Category {
        public TimeCategoryModType type = TimeCategoryModType.UPDATE;
        public int id;
        public String name;
    }

    public static class Code {
        public TimeCategoryModType type = TimeCategoryModType.UPDATE;
        public int categoryIndex;
        public int id;
        public String name;
        public boolean archived;
    }

    public List<Category> categories = new ArrayList<>();
    public List<Code> codes = new ArrayList<>();

    public TimeEntryModify(RequestID requestID) {
        super(requestID);
    }

    @Override
    public PacketType type() {
        return packetType;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        super.writeToOutput(output);

        output.writeInt(categories.size());

        for (Category category : categories) {
            output.writeInt(category.type.ordinal());
            output.writeInt(category.id);
            output.writeShort(category.name.length());
            output.write(category.name.getBytes());
        }

        output.writeInt(codes.size());

        for (Code code : codes) {
            output.writeInt(code.type.ordinal());
            output.writeInt(code.categoryIndex);
            output.writeInt(code.id);
            output.writeShort(code.name.length());
            output.write(code.name.getBytes());
            output.writeByte(code.archived ? 1 : 0);
        }
    }
}
