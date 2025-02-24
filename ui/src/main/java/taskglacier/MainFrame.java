package taskglacier;

import com.formdev.flatlaf.FlatDarkLaf;
import com.formdev.flatlaf.FlatLaf;
import com.formdev.flatlaf.FlatLightLaf;
import data.ServerConnection;
import data.TaskModel;
import dialogs.ConnectToServer;
import io.github.andrewauclair.moderndocking.DockableTabPreference;
import io.github.andrewauclair.moderndocking.app.AppState;
import io.github.andrewauclair.moderndocking.app.Docking;
import io.github.andrewauclair.moderndocking.app.RootDockingPanel;
import io.github.andrewauclair.moderndocking.app.WindowLayoutBuilder;
import io.github.andrewauclair.moderndocking.exception.DockingLayoutException;
import io.github.andrewauclair.moderndocking.ext.ui.DockingUI;
import io.github.andrewauclair.moderndocking.layouts.DockingLayouts;
import io.github.andrewauclair.moderndocking.settings.Settings;
import packets.RequestConfig;
import panels.StatusBar;
import panels.SystemTrayDisplay;
import panels.TasksLists;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.Objects;
import java.util.prefs.Preferences;

public class MainFrame extends JFrame {
    private Thread listen;
    private Socket socket;

    private ServerConnection connection = null;
    private TaskModel taskModel = new TaskModel();
    ImageIcon appIcon16 = new ImageIcon(Objects.requireNonNull(System.getenv("TASK_GLACIER_DEV_INSTANCE") != null ? getClass().getResource("/work-in-progress (1).png") : getClass().getResource("/app-icon-16.png")));
    TrayIcon trayIcon = new TrayIcon(appIcon16.getImage(), "");

    private SystemTrayDisplay systemTrayDisplay = new SystemTrayDisplay(trayIcon);

    public boolean isConnected() {
        return connection != null;
    }

    public ServerConnection getConnection() {
        return connection;
    }

    public TaskModel getTaskModel() {
        return taskModel;
    }

    public MainFrame() throws IOException {
    }
    @Override
    public void setVisible(boolean visible) {
        mainFrame = this;

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        RootDockingPanel root = new RootDockingPanel();
//        root.setVisible(false);
        add(root, gbc);

        setTitle("Task Glacier (Not Connected)");

        StatusBar statusBar = new StatusBar();
        taskModel.addListener(statusBar);

        gbc.gridy++;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        add(statusBar, gbc);
        ImageIcon appIcon64 = new ImageIcon(Objects.requireNonNull(System.getenv("TASK_GLACIER_DEV_INSTANCE") != null ? getClass().getResource("/work-in-progress.png") : getClass().getResource("/app-icon-64.png")));

        setIconImage(appIcon64.getImage());
        if (SystemTray.isSupported()) {
            SystemTray tray = SystemTray.getSystemTray();

            try {
                tray.add(trayIcon);
            } catch (IllegalArgumentException | AWTException e) {
//                throw new RuntimeException(e);
            }
        }

        setSize(600, 400);
        Docking.initialize(this);

        Settings.setDefaultTabPreference(DockableTabPreference.TOP_ALWAYS);

        DockingUI.initialize();
        Docking.registerDockingPanel(root, this);

        new TasksLists(this, "tasks", "Tasks");

        setDefaultCloseOperation(JFrame.HIDE_ON_CLOSE);

        WindowLayoutBuilder builder = new WindowLayoutBuilder("tasks");

        AppState.setDefaultApplicationLayout(builder.buildApplicationLayout());
        DockingLayouts.addLayout("default", builder.buildApplicationLayout());

        setJMenuBar(new MenuBar(this));

        // now that the main frame is set up with the defaults, we can restore the layout
        File layoutFile;
        Preferences preferences;

        if (System.getenv("TASK_GLACIER_DEV_INSTANCE") != null) {
            preferences = Preferences.userNodeForPackage(ConnectToServer.class);
            layoutFile = new File(System.getenv("LOCALAPPDATA") + "/TaskGlacier/dev-layout.xml");
        }
        else {
            preferences = Preferences.userNodeForPackage(MainFrame.class);
            layoutFile = new File(System.getenv("LOCALAPPDATA") + "/TaskGlacier/layout.xml");
        }

        AppState.setPersistFile(layoutFile);

        if (preferences.get("IP-Address", null) != null) {
            try {
                createConnection(preferences.get("IP-Address", null), preferences.getInt("Port", 0));
            }
            catch (Exception ignored) {
            }
        }

        // if we're connected, we'll wait for the request config to complete before restoring
        //if (!isConnected())
        {
            restoreLayout();
        }
        super.setVisible(visible);
    }

    public static void restoreLayout() {
        try {
            AppState.restore();
        } catch (DockingLayoutException e) {
            // something happened trying to load the layout file, record it here
            e.printStackTrace();
        }

        AppState.setAutoPersist(true);
    }

    public void disconnect() {
        if (socket != null) {
            try {
                socket.close();
                connection.close();
            }
            catch (IOException ignored) {
            }
            socket = null;
            connection = null;
            listen = null;

            setTitle("Task Glacier (Not Connected)");

            ((MenuBar) getJMenuBar()).disconnected();
        }
    }

    public void createConnection(String ipAddress, int port) throws IOException {
        socket = new Socket();
        socket.connect(new InetSocketAddress(ipAddress, port), 500);

        setTitle("Task Glacier (Connected - " + ipAddress + ":" + port + ")");

        DataInputStream input = new DataInputStream(socket.getInputStream());
        DataOutputStream output = new DataOutputStream(socket.getOutputStream());

        connection = new ServerConnection(input, output);

        ((MenuBar) getJMenuBar()).connected();

        RequestConfig requestConfig = new RequestConfig();
        connection.sendPacket(requestConfig);

        listen = new Thread(() -> {
            try {
                connection.run(this);
            }
            catch (Exception ignored) {
                disconnect();
            }
        });
        listen.start();
    }

    public static MainFrame mainFrame = null;

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
