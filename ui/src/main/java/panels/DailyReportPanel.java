package panels;

import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DynamicDockableParameters;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.DailyReportMessage;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import java.awt.*;
import java.time.LocalDate;

public class DailyReportPanel extends JPanel implements Dockable {
    @DockingProperty(name = "month", required = true)
    private int month;
    @DockingProperty(name = "month", required = true)
    private int day;
    @DockingProperty(name = "month", required = true)
    private int year;

    private DailyReportMessage.DailyReport report = null;

    private String persistentID;
    private String titleText;
    private String tabText;

    public DailyReportPanel(LocalDate date) {
        month = date.getMonthValue();
        day = date.getDayOfMonth();
        year = date.getYear();

        persistentID = String.format("daily-report-%d-%d-%d", month, day, year);
        titleText = String.format("Daily Report (%d/%d/%d)", month, day, year);
        tabText = String.format("Daily Report (%d/%d/%d)", month, day, year);

        Docking.registerDockable(this);

        buildUI();
    }

    public DailyReportPanel(DynamicDockableParameters parameters) {
        persistentID = parameters.getPersistentID();
        titleText = parameters.getTitleText();
        tabText = parameters.getTabText();

        Docking.registerDockable(this);
    }

    @Override
    public void updateProperties() {
        // TODO request the daily report from the server
    }

    private void buildUI() {
        if (report == null) {
            return;
        }

        JLabel date = new JLabel();
        date.setText(String.format("%d/%d/%d", report.month, report.day, report.year));

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;

        add(date, gbc);
        gbc.gridy++;

        JLabel start = new JLabel();
        JLabel end = new JLabel();
        JLabel total = new JLabel();

        add(start, gbc);
        gbc.gridy++;
        add(end, gbc);
        gbc.gridy++;
        add(total, gbc);
        gbc.gridy++;

        JTree times = new JTree();

        DefaultMutableTreeNode root = new DefaultMutableTreeNode();
        DefaultTreeModel model = new DefaultTreeModel(root);
        times.setModel(model);
        
        add(times, gbc);
        gbc.gridy++;

        revalidate();
        repaint();
    }

    public void update(DailyReportMessage message) {
        report = message.getReport();
        buildUI();
    }

    @Override
    public String getPersistentID() {
        return persistentID;
    }

    @Override
    public String getTitleText() {
        return titleText;
    }

    @Override
    public String getTabText() {
        return tabText;
    }
}
