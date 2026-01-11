package tray;

import com.formdev.flatlaf.FlatClientProperties;
import com.formdev.flatlaf.extras.FlatSVGIcon;
import packets.RequestDailyReport;
import packets.RequestID;
import panels.Search;
import raven.datetime.DatePicker;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import java.awt.*;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.geom.RoundRectangle2D;
import java.time.LocalDate;
import java.util.Objects;

public class SystemTrayDisplay extends JFrame {
    private final TrayIcon trayIcon;
    private final RecentActivity activity;
    private final Search search;
    public final SystemTrayDailyReport dailyReportPanel;

    // https://www.flaticon.com/free-icon/cognitive_8920590?term=cognitive&related_id=8920590
    // https://www.flaticon.com/free-icon/clipboard_1527478?term=task&page=1&position=53&origin=search&related_id=1527478
    // https://www.flaticon.com/free-icon/share_8584964?term=open&page=1&position=3&origin=search&related_id=8584964

    JButton openApp = new JButton(new FlatSVGIcon(Objects.requireNonNull(getClass().getResource("/export-2-svgrepo-com.svg"))).derive(32, 32));//new ImageIcon(Objects.requireNonNull(getClass().getResource("/share.png"))));
    JButton unspecifiedTask = new JButton(new FlatSVGIcon(Objects.requireNonNull(getClass().getResource("/brain-illustration-1-svgrepo-com.svg"))).derive(32, 32));
    JButton dailyReport = new JButton(new FlatSVGIcon(Objects.requireNonNull(getClass().getResource("/report-svgrepo-com.svg"))).derive(32, 32));

    JTextField searchText = new JTextField(30);

    Timer timer;
    MouseEvent e;
    MouseAdapter listener = new MouseAdapter() {
        @Override
        public void mouseClicked(MouseEvent e) {
            if (isVisible()) {
                return;
            }
            SystemTrayDisplay.this.e = e;

            if (e.getClickCount() > 1) {
                MainFrame.mainFrame.setVisible(true);
                timer.stop();
                return;
            }


            if (!isModalDialogShowing() && !timer.isRunning()) {
                timer.start();
            }
        }

        @Override
        public void mouseEntered(MouseEvent e) {
            System.out.println("Mouse entered");
        }
    };

    public SystemTrayDisplay(MainFrame mainFrame, TrayIcon trayIcon) {
        this.trayIcon = trayIcon;
        activity = new RecentActivity(mainFrame);
        search = new Search(mainFrame, false);
        dailyReportPanel = new SystemTrayDailyReport(mainFrame);

        addComponentListener(new ComponentAdapter() {
            // Give the window an elliptical shape.
            // If the window is resized, the shape is recalculated here.
            @Override
            public void componentResized(ComponentEvent e) {
                setShape(new RoundRectangle2D.Double(0, 0, getWidth(), getHeight(), 25, 25));
            }
        });

        FlatSVGIcon searchIcon = new FlatSVGIcon(getClass().getResource("/search-svgrepo-com.svg")).derive(24, 24);

        searchText.putClientProperty(FlatClientProperties.TEXT_FIELD_LEADING_ICON, searchIcon);

        timer = new Timer(250, e1 -> {
            if (isVisible()) {
                // hide
                setVisible(!isVisible());
            }
            else if (e != null) {
                setVisible(true);

                Dimension scrnSize = Toolkit.getDefaultToolkit().getScreenSize();
                Rectangle winSize = GraphicsEnvironment.getLocalGraphicsEnvironment().getMaximumWindowBounds();

                int taskBarHeight = scrnSize.height - winSize.height;
                System.out.println("taskBarHeight = " + taskBarHeight);

                // Get screen dimensions
                Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();

                // Calculate x and y coordinates for the lower-right corner
                int x = screenSize.width - getWidth();
                int y = screenSize.height - getHeight() - taskBarHeight;

                // Set the frame's location
                setLocation(x, y);
            }
        });
        timer.setRepeats(false);
        timer.start();

        trayIcon.addMouseListener(listener);

        setUndecorated(true);
        setType(Type.UTILITY);

        addWindowFocusListener(new WindowAdapter() {
            @Override
            public void windowLostFocus(WindowEvent e) {
                System.out.println("SystemTrayDisplay.windowLostFocus");
                if (!timer.isRunning()) {
                    timer.restart();
                }
            }
        });

        PopupMenu popup = new PopupMenu();
        MenuItem exit = new MenuItem("Exit");

        exit.addActionListener(e -> System.exit(0));

        popup.add(exit);

        trayIcon.setPopupMenu(popup);

        // the frame is undecorated, so we have to do it all ourselves
        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.weightx = 1;
        add(createButtonsPanel(), gbc);
        gbc.gridy++;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        CardLayout layout = new CardLayout();
        JPanel stack = new JPanel(layout);
        stack.add(activity, "activity");
        stack.add(search, "search");
        stack.add(dailyReportPanel, "daily-report");
        
        layout.show(stack, "activity");

        addComponentListener(new ComponentAdapter() {
            @Override
            public void componentHidden(ComponentEvent e) {
                searchText.setText("");
                layout.show(stack, "activity");
            }
        });

        add(stack, gbc);

        setSize(400, 600);

        searchText.getDocument().addDocumentListener(new DocumentListener() {
            @Override
            public void insertUpdate(DocumentEvent e) {
                updateFilter(layout, stack);
            }

            @Override
            public void removeUpdate(DocumentEvent e) {
                updateFilter(layout, stack);
            }

            @Override
            public void changedUpdate(DocumentEvent e) {
                updateFilter(layout, stack);
            }
        });

        dailyReport.addActionListener(e -> {
            // request the daily report
            DatePicker picker = new DatePicker();
            picker.setSelectedDate(LocalDate.now());

            LocalDate localDate = picker.getSelectedDate();

            int year = localDate.getYear();
            int month = localDate.getMonthValue();
            int day = localDate.getDayOfMonth();

            RequestDailyReport request = new RequestDailyReport();
            request.requestID = RequestID.nextRequestID();
            request.month = month;
            request.day = day;
            request.year = year;

            mainFrame.getConnection().sendPacket(request);
            
            // display the system tray daily report panel, which is a simplified version of the full panel
            layout.show(stack, "daily-report");
        });

        unspecifiedTask.addActionListener(e -> mainFrame.startUnspecifiedTask());
    }

    public void setUnspecifiedTaskState(boolean active) {
        unspecifiedTask.setEnabled(active);
    }

    private static boolean isModalDialogShowing() {
        Window[] windows = Window.getWindows();
        if (windows != null) { // don't rely on current implementation, which at least returns [0].
            for (Window w : windows) {
                if (w.isShowing() && w instanceof Dialog && ((Dialog) w).isModal()) {
                    return true;
                }
            }
        }
        return false;
    }

    private void updateFilter(CardLayout layout, JPanel stack) {
        if (searchText.getText().isEmpty()) {
            layout.show(stack, "activity");
        }
        else {
            layout.show(stack, "search");
            search.setSearchText(searchText.getText());
        }
    }

    private JPanel createButtonsPanel() {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;

        openApp.addActionListener(e -> MainFrame.mainFrame.setVisible(true));

        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.weightx = 1;

        panel.add(searchText, gbc);
        gbc.gridx++;

        gbc.fill = GridBagConstraints.NONE;
        gbc.weightx = 0;
        panel.add(dailyReport, gbc);
        gbc.gridx++;

        panel.add(unspecifiedTask, gbc);
        gbc.gridx++;
        gbc.fill = GridBagConstraints.NONE;
        gbc.weightx = 0;

        panel.add(openApp, gbc);

        return panel;
    }
}
