package tray;

import com.formdev.flatlaf.extras.FlatSVGIcon;
import data.Task;
import data.TaskModel;
import packets.TaskInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.time.LocalDate;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.Comparator;
import java.util.Objects;
import java.util.TreeSet;

import static taskglacier.MainFrame.mainFrame;

class RecentActivity extends JPanel implements TaskModel.Listener {
    private static final LocalDate MAX_AGE = LocalDate.now(ZoneId.systemDefault()).minusDays(14);
    private FlatSVGIcon activeIcon = new FlatSVGIcon(Objects.requireNonNull(getClass().getResource("/activity-svgrepo-com.svg"))).derive(24, 24);
    private FlatSVGIcon finishIcon = new FlatSVGIcon(Objects.requireNonNull(getClass().getResource("/checkmark-svgrepo-com.svg"))).derive(24, 24);
    private FlatSVGIcon pendingIcon = new FlatSVGIcon(Objects.requireNonNull(getClass().getResource("/system-pending-line-svgrepo-com.svg"))).derive(24, 24);
    private TreeSet<History> history = new TreeSet<>(Comparator.comparing(o -> ((History) o).session.startTime).reversed());
    private DefaultTableModel model = new DefaultTableModel(0, 1) {
        @Override
        public Class<?> getColumnClass(int columnIndex) {
            return History.class;
        }
    };
    private JTable table = new JTable(model);

    public RecentActivity(MainFrame mainFrame) {
        super(new BorderLayout());
        // build up a history of the last X task changes

        mainFrame.getTaskModel().addListener(this);

        add(new JScrollPane(table));

        table.setRowHeight(table.getRowHeight() * 2);
        table.setDefaultRenderer(History.class, (table, value, isSelected, hasFocus, row, column) -> {
            History history = (History) value;

            JPanel panel = new JPanel(new GridBagLayout());

            GridBagConstraints gbc = new GridBagConstraints();
            gbc.gridx = 0;
            gbc.gridy = 0;

            // icon (pending, active, finished), name, start time - stop time
            JLabel icon = new JLabel();

            switch (history.task.state) {
                case PENDING -> icon.setIcon(pendingIcon);
                case ACTIVE -> icon.setIcon(activeIcon);
                case FINISHED -> icon.setIcon(finishIcon);
            }
            gbc.gridheight = 2;

            panel.add(icon, gbc);

            gbc.gridx++;
            gbc.gridheight = 1;

            JLabel name = new JLabel(history.task.name);
            panel.add(name, gbc);
            gbc.gridy++;

            LocalDate today = LocalDate.now(ZoneId.systemDefault());
            LocalDate yesterday = today.minusDays(1);
            LocalDate instantDate = history.session.startTime.atZone(ZoneId.systemDefault()).toLocalDate();

            if (instantDate.equals(today)) {
                DateTimeFormatter dateTimeFormatter = DateTimeFormatter.ofPattern("hh:mm a");

                JLabel start = new JLabel();
                start.setText(history.session.startTime.atZone(ZoneId.systemDefault()).format(dateTimeFormatter));
                panel.add(start, gbc);
                gbc.gridx++;
            }
            else if (instantDate.equals(yesterday)) {
                JLabel start = new JLabel("Yesterday");
                panel.add(start, gbc);
                gbc.gridx++;
            }
            else {
                JLabel start = new JLabel(String.format("%d days ago", instantDate.datesUntil(today).count()));
                panel.add(start, gbc);
                gbc.gridx++;
            }

            return panel;
        });
    }

    @Override
    public void newTask(Task task) {
        for (TaskInfo.Session session : task.sessions) {
            LocalDate start = session.startTime.atZone(ZoneId.systemDefault()).toLocalDate();

            if (start.isAfter(MAX_AGE)) {
                history.add(new History(task, session));
            }
        }
        update();
    }

    @Override
    public void updatedTask(Task task) {
        for (TaskInfo.Session session : task.sessions) {
            LocalDate start = session.startTime.atZone(ZoneId.systemDefault()).toLocalDate();

            if (start.isAfter(MAX_AGE)) {
                history.add(new History(task, session));
            }
        }
        update();
    }

    @Override
    public void reparentTask(Task task, int oldParent) {
        for (TaskInfo.Session session : task.sessions) {
            LocalDate start = session.startTime.atZone(ZoneId.systemDefault()).toLocalDate();

            if (start.isAfter(MAX_AGE)) {
                history.add(new History(task, session));
            }
        }
        update();
    }

    @Override
    public void configComplete() {
        for (Task task : mainFrame.getTaskModel().getTasks()) {
            for (TaskInfo.Session session : task.sessions) {
                LocalDate start = session.startTime.atZone(ZoneId.systemDefault()).toLocalDate();

                if (start.isAfter(MAX_AGE)) {
                    history.add(new History(task, session));
                }
            }
        }
        update();
    }

    private void update() {
        model.setRowCount(0);

        for (History history1 : history) {
            model.addRow(new Object[]{history1});
        }

        model.fireTableDataChanged();
    }

    private class History {
        Task task;
        TaskInfo.Session session;

        public History(Task task, TaskInfo.Session session) {
            this.task = task;
            this.session = session;
        }

        @Override
        public boolean equals(Object o) {
            if (o == null || getClass() != o.getClass()) {
                return false;
            }
            History history = (History) o;
            return Objects.equals(task, history.task) && Objects.equals(session, history.session);
        }

        @Override
        public int hashCode() {
            return Objects.hash(task, session);
        }
    }
}
