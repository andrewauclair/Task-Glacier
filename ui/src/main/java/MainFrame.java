import com.formdev.flatlaf.FlatDarkLaf;
import com.formdev.flatlaf.FlatLaf;
import com.formdev.flatlaf.FlatLightLaf;
import io.github.andrewauclair.moderndocking.app.AppState;
import io.github.andrewauclair.moderndocking.app.Docking;
import io.github.andrewauclair.moderndocking.app.RootDockingPanel;
import io.github.andrewauclair.moderndocking.exception.DockingLayoutException;
import io.github.andrewauclair.moderndocking.ext.ui.DockingUI;

import javax.swing.*;
import java.awt.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.net.Socket;
import java.time.OffsetDateTime;
import java.util.Arrays;

public class MainFrame extends JFrame {
    private final JMenuItem add;
    private DataOutputStream output;

    public MainFrame() throws IOException {
        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        RootDockingPanel root = new RootDockingPanel();
        add(root, gbc);

        setTitle("Task Glacier (Not Connected)");

        JPanel statusBar = new JPanel(new GridBagLayout());
        JLabel activeTask = new JLabel("No active task");

        gbc.weightx = 0;

        statusBar.add(activeTask, gbc);
        gbc.weightx = 1;
        gbc.gridx=1;
        statusBar.add(new JLabel(), gbc);

        gbc.gridx = 0;
        gbc.gridy++;
        gbc.weightx = 1;
        gbc.weighty = 0;

        add(statusBar, gbc);

        setSize(400, 400);
        Docking.initialize(this);

        DockingUI.initialize();
        Docking.registerDockingPanel(root, this);

//        TasksLists tasks = new TasksLists(output);

        setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);

//        WindowLayoutBuilder tasks1 = new WindowLayoutBuilder("tasks");

//        AppState.setDefaultApplicationLayout(tasks1.buildApplicationLayout());


        JMenuBar menuBar = new JMenuBar();
        setJMenuBar(menuBar);

        JMenu task = new JMenu("Task");
        add = new JMenuItem("Add");
        add.setEnabled(false);
        add.addActionListener(e -> {
            new AddTask(output).setVisible(true);
        });
        task.add(add);

        menuBar.add(task);

        JMenu server = new JMenu("Server");
        JMenuItem connect = new JMenuItem("Connect...");
        connect.addActionListener(e -> {
            class ServerConfig extends JDialog {
                ServerConfig() {
                    GridBagConstraints gbc = new GridBagConstraints();
                    setLayout(new GridBagLayout());
                    gbc.gridx = 0;
                    gbc.gridy = 0;
                    add(new JLabel("IP Address: "), gbc);
                    gbc.gridy++;
                    gbc.gridx++;
                    JTextField IP = new JTextField();
                    add(IP, gbc);
                    gbc.gridx = 0;
                    gbc.gridy++;
                    add(new JLabel("Port: "), gbc);
                    gbc.gridx++;
                    JTextField port = new JTextField();
                    add(port, gbc);

                    gbc.gridy++;
                    JButton connectButton = new JButton("Connect");
                    add(connectButton, gbc);
                    connectButton.addActionListener(e -> {
                        try {
                            createConnection(IP.getText(), Integer.parseInt(port.getText()));
                        } catch (IOException ex) {
                            throw new RuntimeException(ex);
                        }
                    });
                    pack();
                }
            }
            ServerConfig config =new ServerConfig();
            config.setVisible(true);
        });

        server.add(connect);
        menuBar.add(server);

        // now that the main frame is set up with the defaults, we can restore the layout
        File layoutFile = new File("layout.xml");
        AppState.setPersistFile(layoutFile);

        try {
            AppState.restore();
        } catch (DockingLayoutException e) {
            // something happened trying to load the layout file, record it here
            e.printStackTrace();
        }

        AppState.setAutoPersist(true);



    }

    private void createConnection(String ipAddress, int port) throws IOException {
        final Socket socket = new Socket(ipAddress, port);

        setTitle("Task Glacier (Connected)");

        output = new DataOutputStream(socket.getOutputStream());
        add.setEnabled(true);

        Thread listen = new Thread(() -> {
            try (DataInputStream in = new DataInputStream(socket.getInputStream())) {
                int packetLength;
                while ((packetLength = in.readInt()) != -1) {
                    int expectedBytes = packetLength - 4;

                    byte[] bytes = new byte[expectedBytes];

                    int totalRead = 0;

                    while (totalRead < expectedBytes) {
                        int read = in.read(bytes, totalRead, bytes.length - totalRead);
                        if (read == -1) {
                            totalRead = -1;
                            break;
                        }
                        totalRead += read;
                    }

                    if (totalRead == -1) {
                        break;
                    }

                    String source = new String(Arrays.copyOfRange(bytes, 0, bytes.length));
//                    JSONObject obj = new JSONObject(source);
//
//                    switch (obj.getInt("command")) {
//                        case 1: // version request response
//                            System.out.println("Version: " + obj.getString("version"));
//                            break;
//                        case 2: // add task
//                            // this response verifies that the task was added
////                            tasks.addTask(obj.getInt("id"), obj.getString("name"));
//                            break;
//                        case 3: // start task
////                            activeTask.setText(obj.getInt("id") + " - " + obj.getString("name"));
//                            repaint();
//                            break;
//                        case 4: // get task response
//                            OffsetDateTime addTime = OffsetDateTime.parse(obj.getString("add-time"));
//                            System.out.printf("Task %d %s %s%n", obj.getInt("id"), addTime, obj.getString("name"));
//
//                            if (obj.has("start-time")) {
//                                OffsetDateTime startTime = OffsetDateTime.parse(obj.getString("start-time"));
//
//                                System.out.println("start time: " + startTime);
//                            }
//                            break;
//                    }
                }
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        });

        listen.start();
    }
    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            configureLookAndFeel();
            try {
                new MainFrame().setVisible(true);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        });
    }

    private static void configureLookAndFeel() {
        try {
            FlatLaf.registerCustomDefaultsSource( "docking" );

//            System.setProperty("flatlaf.uiScale", String.valueOf(1));

            String lookAndFeel = "dark";
            switch (lookAndFeel) {
                case "light":
                    UIManager.setLookAndFeel(new FlatLightLaf());
                    break;
                case "dark":
                    UIManager.setLookAndFeel(new FlatDarkLaf());
                    break;
                case "github-dark":
//                    UIManager.setLookAndFeel(new FlatGitHubDarkIJTheme());
                    break;
                case "solarized-dark":
//                    UIManager.setLookAndFeel(new FlatSolarizedDarkIJTheme());
                    break;
                default:
                    try {
                        UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
                    } catch (ClassNotFoundException | InstantiationException | IllegalAccessException |
                             UnsupportedLookAndFeelException ex) {
                        throw new RuntimeException(ex);
                    }
                    break;
            }
            FlatLaf.updateUI();
        }
        catch (Exception e) {
            e.printStackTrace();
            try {
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            }
            catch (ClassNotFoundException | InstantiationException | IllegalAccessException |
                   UnsupportedLookAndFeelException ex) {
                throw new RuntimeException(ex);
            }
        }
        UIManager.getDefaults().put("TabbedPane.contentBorderInsets", new Insets(0,0,0,0));
        UIManager.getDefaults().put("TabbedPane.tabsOverlapBorder", true);

    }
}
