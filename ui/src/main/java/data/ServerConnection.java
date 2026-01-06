package data;

import dialogs.About;
import dialogs.AddTask;
import dialogs.UnspecifiedTask;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.BugzillaInfo;
import packets.DailyReportMessage;
import packets.ErrorMessage;
import packets.FailureResponse;
import packets.Packet;
import packets.PacketType;
import packets.RequestDailyReport;
import packets.RequestID;
import packets.RequestWeeklyReport;
import packets.TaskInfo;
import packets.TimeCategoriesMessage;
import packets.Version;
import packets.WeeklyReport;
import panels.DailyReportPanel;
import panels.WeeklyReportPanel;
import taskglacier.MainFrame;

import javax.swing.*;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class ServerConnection {
    private final DataInputStream input;
    private final DataOutputStream output;
    List<Packet> toSend = new ArrayList<>();

    public ServerConnection(DataInputStream input, DataOutputStream output) {
        this.input = input;
        this.output = output;
    }

    public void sendPacketWhenReady(Packet packet) {
        if (output != null) {
            sendPacket(packet);
        }
        else {
            toSend.add(packet);
        }
    }

    // call this from a thread
    public void run(MainFrame mainFrame) {
        try {
            int packetLength;
            while ((packetLength = input.readInt()) != -1) {
                int expectedBytes = packetLength - 4;

                byte[] bytes = new byte[expectedBytes];

                int totalRead = 0;

                while (totalRead < expectedBytes) {
                    int read = input.read(bytes, totalRead, bytes.length - totalRead);
                    if (read == -1) {
                        totalRead = -1;
                        break;
                    }
                    totalRead += read;
                }

                if (totalRead == -1) {
                    break;
                }

                PacketType packetType = PacketType.valueOf(ByteBuffer.wrap(bytes, 0, 4).getInt());
                System.out.println("Received packet with length: " + packetLength + ", type: " + packetType);

                if (packetType == PacketType.VERSION) {
                    Version version = Version.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);
                    About.serverVersion = version.version;
                }
                if (packetType == PacketType.TASK_INFO) {
                    TaskInfo info = TaskInfo.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    SwingUtilities.invokeLater(() -> mainFrame.getTaskModel().receiveInfo(info));
                }
                else if (packetType == PacketType.REQUEST_CONFIGURATION_COMPLETE) {
                    SwingUtilities.invokeLater(MainFrame::restoreLayout);

                    SwingUtilities.invokeLater(() -> mainFrame.getTaskModel().configurationComplete());

                    for (Packet packet : toSend) {
                        sendPacket(packet);
                    }
                    toSend.clear();
                }
                else if (packetType == PacketType.UNSPECIFIED_TASK_ACTIVE) {
                    SwingUtilities.invokeLater(() -> mainFrame.unspecifiedTaskActive());
                }
                else if (packetType == PacketType.BUGZILLA_INFO) {
                    BugzillaInfo info = BugzillaInfo.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    MainFrame.bugzillaInfo.put(info.name, info);
                }
                else if (packetType == PacketType.DAILY_REPORT) {
                    DailyReportMessage dailyReport = DailyReportMessage.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    SwingUtilities.invokeLater(() -> mainFrame.receivedDailyReport(dailyReport));
                }
                else if (packetType == PacketType.WEEKLY_REPORT) {
                    WeeklyReport report = WeeklyReport.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    SwingUtilities.invokeLater(() -> mainFrame.receivedWeeklyReport(report));
                }
                else if (packetType == PacketType.TIME_CATEGORIES_DATA) {
                    TimeCategoriesMessage message = TimeCategoriesMessage.parse(new DataInputStream(new ByteArrayInputStream(bytes)), packetLength);

                    mainFrame.getTimeData().processPacket(message);
                }
                else if (packetType == PacketType.FAILURE_RESPONSE) {
                    FailureResponse failure = FailureResponse.parse(new DataInputStream((new ByteArrayInputStream(bytes))), packetLength);

                    SwingUtilities.invokeLater(() -> {
                        if (AddTask.openInstance != null) {
                            AddTask.openInstance.failureResponse(failure.message);
                        }
                        else {
                            JOptionPane.showMessageDialog(mainFrame, failure.message, "Failure", JOptionPane.ERROR_MESSAGE);
                        }
                    });
                }
                else if (packetType == PacketType.SUCCESS_RESPONSE) {
                    SwingUtilities.invokeLater(() -> {
                        int requestID = ByteBuffer.wrap(bytes, 4, 4).getInt();

                        if (AddTask.openInstance != null && AddTask.activeRequests.contains(requestID)) {
                            AddTask.activeRequests.remove((Integer) requestID);

                            if (AddTask.activeRequests.isEmpty()) {
                                AddTask.openInstance.close();
                            }
                        }
                        if (UnspecifiedTask.openInstance != null && UnspecifiedTask.requestID == requestID) {
                            UnspecifiedTask.openInstance.close();
                        }
                    });
                }
                else if (packetType == PacketType.ERROR_MESSAGE) {
                    ErrorMessage error = ErrorMessage.parse(new DataInputStream((new ByteArrayInputStream(bytes))), packetLength);

                    SwingUtilities.invokeLater(() -> {
                        JOptionPane.showMessageDialog(mainFrame, error.message, "Error", JOptionPane.ERROR_MESSAGE);
                    });
                }
            }
        }
        catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public void close() throws IOException {
        input.close();
        output.close();
    }

    public boolean sendPacket(Packet packet) {
        if (output != null) {
            if (List.of(PacketType.START_TASK, PacketType.STOP_TASK, PacketType.FINISH_TASK, PacketType.UPDATE_TASK, PacketType.MOVE_TASK).contains(packet.type())) {
                for (Dockable dockable : Docking.getDockables()) {
                    if (dockable instanceof DailyReportPanel dailyReport) {
                        RequestDailyReport request = new RequestDailyReport();
                        request.requestID = RequestID.nextRequestID();
                        request.month = dailyReport.getMonth();
                        request.day = dailyReport.getDay();
                        request.year = dailyReport.getYear();

                        SwingUtilities.invokeLater(() -> sendPacket(request));
                    }
                    else if (dockable instanceof WeeklyReportPanel weeklyReport) {
                        RequestWeeklyReport request = new RequestWeeklyReport();
                        request.requestID = RequestID.nextRequestID();
                        request.month = weeklyReport.getMonth();
                        request.day = weeklyReport.getDay();
                        request.year = weeklyReport.getYear();

                        SwingUtilities.invokeLater(() -> sendPacket(request));
                    }
                }
            }
            
            try {
                try (ByteArrayOutputStream output = new ByteArrayOutputStream(packet.size())) {
                    packet.writeToOutput(new DataOutputStream(output));
                }

                packet.writeToOutput(this.output);
                System.out.println("Sent packet with size: " + packet.size() + ", type: " + packet.type());
                return true;
            }
            catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
        return false;
    }
}
