package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Version implements Packet {
    public String version;

    @Override
    public int size() {
        return 8 + 2 + version.length();
    }

    @Override
    public PacketType type() {
        return PacketType.VERSION;
    }

    public static Version parse(DataInputStream input, int size) throws IOException {
        Version version = new Version();

        input.readInt(); // packet type
        version.version = Packet.parseString(input);

        return version;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {

    }
}
