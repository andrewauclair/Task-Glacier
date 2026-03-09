package packets;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Version implements Packet {
    public String version;

    @Override
    public PacketType type() {
        return PacketType.VERSION;
    }

    public static Version parse(DataInputStream input) throws IOException {
        Version version = new Version();

        version.version = Packet.parseString(input);

        return version;
    }

    @Override
    public void writeToOutput(DataOutputStream output) throws IOException {

    }
}
