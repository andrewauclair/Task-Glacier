package panels;

import data.Task;
import data.TaskState;
import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.DockingProperty;
import io.github.andrewauclair.moderndocking.DynamicDockableParameters;
import io.github.andrewauclair.moderndocking.app.Docking;
import net.byteseek.swing.treetable.TreeUtils;
import taskglacier.MainFrame;
import tree.TaskTreeTable;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;

public class TasksList extends JPanel implements Dockable {
    public JTextField search = new JTextField(20);
    @DockingProperty(name = "taskID", required = true)
    private int taskID = 0;
    @DockingProperty(name = "allTasks", required = true)
    private boolean allTasks = false;
    private String persistentID;
    private String titleText;
    private String tabText;
    private MainFrame mainFrame;
    private TaskTreeTable taskTable;

    public TasksList(MainFrame mainFrame) {
        allTasks = true;

        this.mainFrame = mainFrame;
        this.persistentID = "tasks";
        this.titleText = "Tasks";
        this.tabText = "Tasks";

        Docking.registerDockable(this);

        Task rootObject = new Task(0, 0, "");
        configure(rootObject);
    }

    public TasksList(MainFrame mainFrame, Task task) {
        taskID = task.id;
        this.mainFrame = mainFrame;
        persistentID = "task-list-" + task.id;
        titleText = "Tasks (" + task.name + ")";
        tabText = "Tasks (" + task.name + ")";

        Docking.registerDockable(this);

        configure(task);
    }

    public TasksList(DynamicDockableParameters parameters) {
        super(new BorderLayout());

        this.persistentID = parameters.getPersistentID();
        this.titleText = parameters.getTitleText();
        this.tabText = parameters.getTabText();

        mainFrame = MainFrame.mainFrame;

        Docking.registerDockable(this);

        Task rootObject = new Task(0, 0, "");
        configure(rootObject);
    }

    @Override
    public void updateProperties() {
        System.out.println("AltTasksList.updateProperties");
        mainFrame = MainFrame.mainFrame;

        if (allTasks) {
        }
        else {
            Task task = mainFrame.getTaskModel().getTask(taskID);

            if (task != null) {
                configure(task);
            }
            else {
            }
        }
    }

    private void configure(Task rootObject) {
        taskTable = new TaskTreeTable(mainFrame, rootObject, taskID, false);

        // Register the keyboard shortcut
        KeyStroke keyStroke = KeyStroke.getKeyStroke(KeyEvent.VK_F, KeyEvent.CTRL_DOWN_MASK);
        getInputMap(JComponent.WHEN_ANCESTOR_OF_FOCUSED_COMPONENT).put(keyStroke, "search");
        getActionMap().put("search", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                search.requestFocus();
            }
        });

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(new JScrollPane(taskTable), gbc);
        gbc.gridy++;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        add(search, gbc);

        search.getDocument().addDocumentListener(new DocumentListener() {
            @Override
            public void insertUpdate(DocumentEvent e) {
                setSearchText(search.getText());
            }

            @Override
            public void removeUpdate(DocumentEvent e) {
                setSearchText(search.getText());
            }

            @Override
            public void changedUpdate(DocumentEvent e) {
                setSearchText(search.getText());
            }
        });
    }

    public void setSearchText(final String search) {
        taskTable.setNodeFilter(treeNode -> {
            Task obj = TreeUtils.getUserObject(treeNode);
            boolean includeFinish = search.startsWith("finish: ");
            String text = search;

            if (includeFinish) {
                text = text.substring("finish: ".length());
                return !childrenHaveMatch(obj, text);
            }
            return obj.state == TaskState.FINISHED || !childrenHaveMatch(obj, text);
        });
    }

    private boolean childrenHaveMatch(Task obj, String text) {
        for (Task child : obj.children) {
            if (childrenHaveMatch(child, text)) {
                return true;
            }
        }
        return obj.name.toLowerCase().contains(text.toLowerCase());
    }

    @Override
    public String getPersistentID() {
        return persistentID;
    }

    @Override
    public String getTabText() {
        return tabText;
    }

    @Override
    public String getTitleText() {
        return titleText;
    }

    @Override
    public boolean isWrappableInScrollpane() {
        return false;
    }

    @Override
    public boolean isClosable() {
        return !persistentID.equals("tasks");
    }
}
