package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

public class BugzillaInfo implements Packet {
    public final int instanceID;
    public final String name;
    public final String url;
    public final String apiKey;
    public final String username;
    public int rootTaskID;
    public List<String> groupTasksBy = new ArrayList<>();
    public Map<String, String> labelToField = new HashMap<>();

    private int size = 0;

    public BugzillaInfo(int instanceID, String name, String url, String apiKey, String username) {
        this.instanceID = instanceID;
        this.name = name;
        this.url = url;
        this.apiKey = apiKey;
        this.username = username;
    }

    public static BugzillaInfo parse(DataInputStream input, int size) throws IOException {
        input.readInt(); // packet type
        int instanceID = input.readInt();
        String name = Packet.parseString(input);
        String url = Packet.parseString(input);
        String apiKey = Packet.parseString(input);
        String username = Packet.parseString(input);

        BugzillaInfo message = new BugzillaInfo(instanceID, name, url, apiKey, username);
        message.size = size;
        message.rootTaskID = input.readInt();

        int groupTasksByCount = input.readInt();

        for (int i = 0; i < groupTasksByCount; i++) {
            message.groupTasksBy.add(Packet.parseString(input));
        }

        int labelToFieldCount = input.readInt();

        for (int i = 0; i < labelToFieldCount; i++) {
            String label = Packet.parseString(input);
            String field = Packet.parseString(input);

            message.labelToField.put(label, field);
        }

        return message;
    }

    @Override
    public int size() {
        return size;
    }

    @Override
    public PacketType type() {
        return PacketType.BUGZILLA_INFO;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        AtomicInteger size = new AtomicInteger(16); // size, packet type, instance ID, and root task ID
        size.addAndGet(2 + name.length());
        size.addAndGet(2 + url.length());
        size.addAndGet(2 + apiKey.length());
        size.addAndGet(2 + username.length());

        size.addAndGet(4);
        groupTasksBy.forEach(s -> {
            size.addAndGet(2);
            size.addAndGet(s.length());
        });
        size.addAndGet(4);
        labelToField.forEach((s, s2) -> {
            size.addAndGet(4);
            size.addAndGet(s.length());
            size.addAndGet(s2.length());
        });
        this.size = size.get();

        output.writeInt(size.get());
        output.writeInt(PacketType.BUGZILLA_INFO.value());
        output.writeInt(instanceID);
        Packet.writeString(output, name);
        Packet.writeString(output, url);
        Packet.writeString(output, apiKey);
        Packet.writeString(output, username);
        output.writeInt(rootTaskID);

        output.writeInt(groupTasksBy.size());

        for (String groupBy : groupTasksBy) {
            Packet.writeString(output, groupBy);
        }

        output.writeInt(labelToField.keySet().size());

        for (String label : labelToField.keySet()) {
            String field = labelToField.get(label);

            Packet.writeString(output, label);
            Packet.writeString(output, field);
        }
    }

    public int getRootTaskID() {
        return rootTaskID;
    }

    public void setRootTaskID(int rootTaskID) {
        this.rootTaskID = rootTaskID;
    }

    public Map<String, String> getLabelToField() {
        return labelToField;
    }

    public void setLabelToField(Map<String, String> labelToField) {
        this.labelToField = labelToField;
    }
}
