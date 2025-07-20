package tray;

import data.Task;
import data.TaskModel;
import packets.TaskInfo;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.util.Comparator;
import java.util.Objects;
import java.util.TreeSet;

import static taskglacier.MainFrame.mainFrame;

class RecentActivity extends JPanel implements TaskModel.Listener {
    private static final int MAX_HISTORY = 50;

    @Override
    public void newTask(Task task) {
        for (TaskInfo.Session session : task.sessions) {
            history.add(new History(task, session));
        }
        update();
    }

    @Override
    public void updatedTask(Task task) {
        for (TaskInfo.Session session : task.sessions) {
            history.add(new History(task, session));
        }
        update();
    }

    @Override
    public void reparentTask(Task task, int oldParent) {
        for (TaskInfo.Session session : task.sessions) {
            history.add(new History(task, session));
        }
        update();
    }

    @Override
    public void configComplete() {
        for (Task task : mainFrame.getTaskModel().getTasks()) {
            for (TaskInfo.Session session : task.sessions) {
                history.add(new History(task, session));
            }
        }
        update();
    }

    private void update() {
        model.setRowCount(0);

        for (History history1 : history) {
            model.addRow(new Object[] { history1.task.name, history1.session.startTime });
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
            if (o == null || getClass() != o.getClass()) return false;
            History history = (History) o;
            return Objects.equals(task, history.task) && Objects.equals(session, history.session);
        }

        @Override
        public int hashCode() {
            return Objects.hash(task, session);
        }
    }

    private TreeSet<History> history = new TreeSet<>(Comparator.comparing(o -> ((History) o).session.startTime).reversed());
    private DefaultTableModel model = new DefaultTableModel(0, 2);
    private JTable table = new JTable(model);

    public RecentActivity(MainFrame mainFrame) {
        super(new BorderLayout());
        // build up a history of the last X task changes

        mainFrame.getTaskModel().addListener(this);

        add(new JScrollPane(table));
    }
}
