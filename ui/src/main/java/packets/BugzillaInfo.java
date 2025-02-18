package packets;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

public class BugzillaInfo implements Packet {
    private final String url;
    private final String apiKey;
    private final String username;
    private String groupTasksBy;
    private int rootTaskID;
    private Map<String, String> labelToField = new HashMap<>();

    public BugzillaInfo(String url, String apiKey, String username) {
        this.url = url;
        this.apiKey = apiKey;
        this.username = username;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {
        AtomicInteger size = new AtomicInteger(24 + url.length() + apiKey.length() + username.length());
        size.addAndGet(groupTasksBy.length());
        labelToField.forEach((s, s2) -> {
            size.addAndGet(4);
            size.addAndGet(s.length());
            size.addAndGet(s2.length());
        });
        output.writeInt(size.get());
        output.writeInt(PacketType.BUGZILLA_INFO.value());
        output.writeShort((short) url.length());
        output.write(url.getBytes());
        output.writeShort((short) apiKey.length());
        output.write(apiKey.getBytes());
        output.writeShort((short) username.length());
        output.write(username.getBytes());
        output.writeInt(rootTaskID);
        output.writeShort(groupTasksBy.length());
        output.write(groupTasksBy.getBytes());

        output.writeInt(labelToField.keySet().size());

        for (String label : labelToField.keySet()) {
            String field = labelToField.get(label);

            output.writeShort(label.length());
            output.write(label.getBytes());
            output.writeShort(field.length());
            output.write(field.getBytes());
        }
    }

    public String getGroupTasksBy() {
        return groupTasksBy;
    }

    public void setGroupTasksBy(String groupTasksBy) {
        this.groupTasksBy = groupTasksBy;
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
